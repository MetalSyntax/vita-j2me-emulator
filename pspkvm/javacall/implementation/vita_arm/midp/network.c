/* vita_arm/midp/network.c — Fase 3: port PSP -> Vita.
 *
 * Origen: psp_mips/midp/network.c
 * Cambios:
 *  - Init PSP (sceNetInit 5 args + InetInit + ResolverInit + ApctlInit +
 *    AddHandler) -> Vita sceNetInit(&SceNetInitParam) + sceNetCtlInit.
 *    El pool de SceNetInitParam.memory se reserva con
 *    sceKernelAllocMemBlock (bug#6: un buffer estático/BSS crashea
 *    dentro del propio sceNetInit en HW real, ver comentario abajo).
 *    No hay Inet/Apctl separados en Vita.
 *  - isNetConnected: sceNetApctlGetState()==4 -> sceNetCtlInetGetState()
 *    == SCE_NETCTL_STATE_CONNECTED. La Vita se conecta vía ajustes del
 *    sistema; no hay diálogo de selección de AP (netDialog eliminado).
 *  - Perfiles/proxy Apctl (connect_profile, http_proxy): sin equivalente
 *    en Vita -> devuelven FAIL de forma segura. Soul of Darkness es
 *    offline; el online real queda para Fase 6 si hace falta.
 *  - Resolución DNS: sceNetResolver* (API distinta en Vita) -> getaddrinfo
 *    de newlib en el hilo resolver existente. Se conserva la caché DNS,
 *    el watchdog de 15s (javacall_time_* sin cambios) y getsockopt.
 *  - stricmp (PSP) -> strcasecmp (POSIX/newlib).
 *  - Hilo resolver: CreateThread 6 args PSP -> 7 args Vita.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_socket.h"
#include "javacall_network.h"
#include "javacall_logging.h"

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#define DEBUG_JAVACALL_NETWORK 1

#define BYPASS_CMWAP_GATEWAY 1

/* 1 = usa SceNet de verdad; 0 = arranca sin red (ver bug#6). */
#ifndef VITA_NET_LIVE
#define VITA_NET_LIVE 1
#endif

struct _DNSHandle;
#define MAX_HOST_LENGTH 256
#define MAX_IPBYTES_SIZE 32
#define DNS_CACHE_SIZE 16

/* Pool de memoria para la pila de red Vita.
 * Fase 2b-bug#4: 64K provocaba crash dentro de sceNetInit en HW real.
 * Fase 2b-bug#6 (causa raíz real, no el tamaño): SceNetInitParam.memory
 * NO puede ser un buffer estático/BSS por más alineado que esté --
 * sceNetInit lo registra con el driver de red, que exige memoria de un
 * SceKernelMemBlock de verdad (gotcha clásico y muy documentado de
 * vitasdk: mismo motivo por el que sceGxm/sceAudio piden memblocks en
 * vez de punteros cualquiera). Con un array estático, sceNetInit salta
 * a una dirección inválida dentro del propio syscall (PC=0x0 en el
 * crash dump) apenas arranca a usar ese puntero como si fuera memoria
 * de kernel. Se reserva ahora con sceKernelAllocMemBlock. */
#define VITA_NET_POOL_SIZE (256*1024)
static SceUID vita_net_pool_uid = -1;
static void *vita_net_pool = NULL;

static int vita_net_pool_alloc(void) {
    vita_net_pool_uid = sceKernelAllocMemBlock("VitaJ2MENetPool",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, VITA_NET_POOL_SIZE, NULL);
    if (vita_net_pool_uid < 0) {
        javacall_printf("VITAJ2ME-TRACE: sceKernelAllocMemBlock (net) fallo %08X\n",
                        (unsigned)vita_net_pool_uid);
        return -1;
    }
    if (sceKernelGetMemBlockBase(vita_net_pool_uid, &vita_net_pool) < 0
        || vita_net_pool == NULL) {
        javacall_printf("VITAJ2ME-TRACE: sceKernelGetMemBlockBase (net) fallo\n");
        sceKernelFreeMemBlock(vita_net_pool_uid);
        vita_net_pool_uid = -1;
        vita_net_pool = NULL;
        return -1;
    }
    return 0;
}

typedef struct _DNSHandle{
	struct _DNSHandle* next;
	int ok;
	int ip_len;
	int max_ip_len;
	SceUID res_thread_id;
	javacall_handle monitor_handle;
	char ip[MAX_IPBYTES_SIZE];
	char hostname[MAX_HOST_LENGTH];
} DNSHandle;

typedef struct _DNS_CACHE{
	char hostname[MAX_HOST_LENGTH];
	char ipbytes[32];
	int    len;
	int    hit;
} DNS_CACHE;

static int network_initialized = 0;
static DNSHandle* dnsqueue = NULL;
static DNS_CACHE dns_cache[DNS_CACHE_SIZE]={0};

extern void socket_read_buffer_init();

static int isNetConnected() {
#if !VITA_NET_LIVE
    return 0; /* sin pila de red: siempre offline (graceful) */
#else
    int state = SCE_NETCTL_STATE_DISCONNECTED;
	if (sceNetCtlInetGetState(&state) != 0) {
		return 0;
	}
#ifdef DEBUG_JAVACALL_NETWORK
    	javacall_printf("isNetConnected: state %d\n", state);
#endif

	return state == SCE_NETCTL_STATE_CONNECTED ? 1 : 0;
#endif
}

/**
 * Performs platform-specific initialization of the networking system.
 * Will be called ONCE during VM startup before opening a network connection.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK caller must call the finish function to
 * 			complete the operation
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_start(void) {
    int err;
    /* Canario anti stack-smash (bug#5): se verifica en cada salida. */
    volatile unsigned canary = 0xDEADBEEF;

    javacall_printf("VITAJ2ME-TRACE: network_init_start >>\n");
    if (network_initialized) {
        javacall_printf("VITAJ2ME-TRACE: network_init_start << already\n");
    	return JAVACALL_OK;
    }

#if !VITA_NET_LIVE
    /* Kill switch manual (dejar en 0 arranca sin red, offline
     * garantizado) -- ver el fix de bug#6 más abajo antes de tocar
     * esto: el problema real ya está identificado y corregido. */
    javacall_printf("VITAJ2ME-TRACE: red desactivada (VITA_NET_LIVE=0)\n");
    socket_read_buffer_init();
    network_initialized = 1;
    javacall_printf("VITAJ2ME-TRACE: network_init_start << OK (sin red)\n");
    return JAVACALL_OK;
#endif

    /* Fase 2b-bug#6 (segunda parte): el memblock del pool arregló que
     * sceNetInit reciba memoria valida (se confirmo en HW: el puntero
     * ahora sale de un memblock real, con direccion distinta en cada
     * arranque -- antes era la misma direccion fija de un array
     * estatico), pero SIGUE crasheando con PC=0x0 dentro del propio
     * sceNetInit. Causa: el modulo SceNet no esta cargado. Los stubs
     * .vitalink.fstubs.SceNet/.SceNetCtl solo resuelven el NID contra
     * la tabla de importacion; si el modulo no esta cargado en el
     * proceso esa entrada no apunta a codigo real todavia -> salto a
     * 0x0 apenas el syscall intenta usarlo. Hace falta pedirlo
     * explicitamente antes de usar CUALQUIER API de SceNet/SceNetCtl. */
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) != 0) {
        if ((err = sceSysmoduleLoadModule(SCE_SYSMODULE_NET)) < 0) {
            javacall_printf(": Error, no se pudo cargar SCE_SYSMODULE_NET %08X\n",
                            err);
            return JAVACALL_FAIL;
        }
    }

    if (vita_net_pool == NULL && vita_net_pool_alloc() < 0) {
        javacall_printf(": Error, no se pudo reservar el pool de red\n");
        return JAVACALL_FAIL;
    }

    SceNetInitParam initparam;
    initparam.memory = vita_net_pool;
    initparam.size = VITA_NET_POOL_SIZE;
    initparam.flags = 0;
    javacall_printf("VITAJ2ME-TRACE: sceNetInit(pool=%p, %u)...\n",
                    vita_net_pool, (unsigned)VITA_NET_POOL_SIZE);
    if ((err = sceNetInit(&initparam)) < 0) {
    	javacall_printf(": Error, could not sceNetInit the network %08X\n", err);
	return JAVACALL_FAIL;
    }
    javacall_printf("VITAJ2ME-TRACE: sceNetInit OK, sceNetCtlInit()...\n");
    if ((err = sceNetCtlInit()) < 0) {
    	javacall_printf(": Error, could not sceNetCtlInit the network %08X\n", err);
	return JAVACALL_FAIL;
    }
    javacall_printf("VITAJ2ME-TRACE: net up, init read buffer\n");

    socket_read_buffer_init();

    network_initialized = 1;
    if (canary != 0xDEADBEEF) {
        javacall_printf("VITAJ2ME-TRACE: CANARY SMASHED %08x!\n", canary);
    }
    javacall_printf("VITAJ2ME-TRACE: network_init_start << OK\n");
    return JAVACALL_OK;
}

char* javacall_network_get_profile(int index) {
    (void)index;
    /* Vita: sin perfiles elegibles por la app (conexión vía ajustes). */
    return NULL;
}

javacall_result javacall_network_connect_profile(int index) {
    (void)index;
    return JAVACALL_FAIL;
}

javacall_result javacall_network_connect_state(int* state) {
    int st = SCE_NETCTL_STATE_DISCONNECTED;
    int err = sceNetCtlInetGetState(&st);
    if (err != 0)		{
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_printf(": sceNetCtlInetGetState returns $%x\n", err);
#endif
        return JAVACALL_FAIL;
    } else {
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_printf(" sceNetCtlInetGetState state %d\n", st);
#endif
        *state = (st == SCE_NETCTL_STATE_CONNECTED) ? 4 : st;
        return JAVACALL_OK;
    }
}

void javacall_network_disconnect() {
    /* Vita: la conexión la gestiona el sistema; nada que colgar. */
}

/**
 * Finishes platform-specific initialization of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_UP
 * notification from the platform.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK caller must call the finish function to
 * 			complete the operation
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_init_finish(void) {
    return JAVACALL_OK;
}

/**
 * Performs platform-specific finalization of the networking system.
 * Will be called ONCE during VM shutdown.
 *
 * @retval JAVACALL_WOULD_BLOCK caller must call xxx_finalize_finish
 *         function to complete the operation
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_start(void){
    return JAVACALL_OK;
}

/**
 * Finishes platform-specific finalize of the networking system.
 * The function is invoked be the JVM after receiving JAVACALL_NETWORK_DOWN
 * notification from the platform.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_network_finalize_finish(void)
{
    return JAVACALL_FAIL;
}

static javacall_result lookup_dns_cache(char *hostname,
    /*OUT*/ unsigned char *pAddress, int maxLen, /*OUT*/ int *pLen) {
    int i;
    for (i = 0; i < DNS_CACHE_SIZE; i++) {
        if (strcasecmp(dns_cache[i].hostname, hostname) == 0) {
            if (dns_cache[i].len <= maxLen) {
                memcpy(pAddress, dns_cache[i].ipbytes, dns_cache[i].len);
                *pLen = dns_cache[i].len;
                dns_cache[i].hit++;
#ifdef DEBUG_JAVACALL_NETWORK
                javacall_printf("dns cache hit\n");
#endif
                return JAVACALL_OK;
            }
        }
    }
#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("dns cache miss\n");
#endif
    return JAVACALL_FAIL;
}

static void add_dns_cache(char* hostname, unsigned char*ipbytes, int len) {
    int i;
    unsigned int mh = (unsigned int)-1;
    int selected = 0;

    if (len > MAX_IPBYTES_SIZE) {
    	javacall_printf("too long ipbytes that cannot add into dns cache\n");
    	return;
    }

    for (i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].hostname[0] == '\0') {
            selected = i;
            break;
        } else {
            if ((unsigned int)dns_cache[i].hit < mh) {
                mh = (unsigned int)dns_cache[i].hit;
                selected = i;
            }
        }
    }

    strncpy(dns_cache[selected].hostname, hostname, MAX_HOST_LENGTH);
    memcpy(dns_cache[selected].ipbytes, ipbytes, len);
    dns_cache[selected].len = len;
#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("add to dns cache %d\n", selected);
#endif
}

void monitor_timeout(javacall_handle handle) {
	DNSHandle* p;
        for (p = dnsqueue; p != 0; p = p->next) {
        	if (p->monitor_handle == handle) {
        		/* getaddrinfo bloqueante sin cancelación: el watchdog solo
        		 * marca; el hilo termina solo al resolver/fallar. */
        		p->ok = -2;
        		return;
        	}
        }
}

int resolve_thread(SceSize args, void *argp) {
       (void)args;
       DNSHandle* hdns = *(DNSHandle**)argp;
       int ret = -1;

       do {

             if (hdns == NULL) {
             		javacall_print("FATAL: resolve_thread: passed DNSHandle* is NULL\n");
             		break;
             }
              int retry = 3;
             	javacall_handle monitor_handle;
         		if (JAVACALL_FAIL ==
         			javacall_time_initialize_timer(15000, JAVACALL_FALSE,
         										monitor_timeout, &monitor_handle)) {

        			javacall_print("Error, could not create monitor timer\n");
        			break;
         		}
         		hdns->monitor_handle = monitor_handle;

             struct addrinfo hints, *res = NULL;
             memset(&hints, 0, sizeof(hints));
             hints.ai_family = AF_INET;
             hints.ai_socktype = SOCK_STREAM;
             do {
                 if (res) {
                     freeaddrinfo(res);
                     res = NULL;
                 }
                 ret = getaddrinfo(hdns->hostname, NULL, &hints, &res);
#ifdef DEBUG_JAVACALL_NETWORK
                 javacall_printf("getaddrinfo(%s) return %d\n", hdns->hostname, ret);
#endif
             } while (--retry > 0 && ret != 0);

         	 javacall_time_finalize_timer(monitor_handle);

        	 if (ret != 0 || res == NULL) {
#ifdef DEBUG_JAVACALL_NETWORK
        		javacall_printf("Error resolving %s\n", hdns->hostname);
#endif
        		break;
        	 }

        	 struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        	 int len = sizeof(sin->sin_addr.s_addr);
        	 if (len <= hdns->max_ip_len) {
#ifdef DEBUG_JAVACALL_NETWORK
        		javacall_printf("Resolved %s\n", hdns->hostname);
#endif
        		hdns->ip_len = len;
        		memcpy(hdns->ip, &sin->sin_addr.s_addr, len);
        		hdns->ok = 1;
        		add_dns_cache(hdns->hostname, (unsigned char*)hdns->ip, hdns->ip_len);
        		freeaddrinfo(res);
        		javanotify_socket_event(JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED,
        								hdns, JAVACALL_OK);
        		sceKernelExitDeleteThread(0);
        		return 0;
        	 } else {
        		javacall_print("FATAL:Not enough space to put ip string\n");
        		freeaddrinfo(res);
        		break;
        	 }
       } while(0);

		javanotify_socket_event(JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED,
        								hdns, JAVACALL_FAIL);

       sceKernelExitDeleteThread(-1);

       return -1;
}

static void* start_lookup_ip(char* hostname, int maxIpLen) {
	SceUID thid = sceKernelCreateThread("resolver_thread", resolve_thread,
					    0x40, 16 * 1024, 0, 0, 0);
	if(thid < 0) {
		javacall_printf("Error, could not create thread\n");
		return NULL;
	}

	DNSHandle* hdns = (DNSHandle*)malloc(sizeof(DNSHandle));
	if (hdns == NULL) {
		javacall_printf("Error, could not allocate memory for DNSHandle\n");
		return NULL;
	}
	hdns->res_thread_id = thid;
	strncpy(hdns->hostname, hostname, sizeof(hdns->hostname));
	hdns->hostname[ sizeof(hdns->hostname) - 1 ] = '\0';
	hdns->ok = 0;
	hdns->max_ip_len = maxIpLen;
	hdns->next = dnsqueue;
	dnsqueue = hdns;
	sceKernelStartThread(thid, sizeof(&hdns), &hdns);
	return hdns;
}

static int end_lookup_ip(DNSHandle* handle, char* pAddress, int maxLen, int* pLen) {
	//SceUID thid = handle->res_thread_id;

	int ok = handle->ok;
	if (ok > 0) {
        	*pLen = handle->ip_len;
        	memcpy(pAddress, handle->ip, *pLen);
	}

	if (dnsqueue == handle) {
		dnsqueue = handle->next;
	} else {
        	DNSHandle* p;
        	for (p = dnsqueue; p != 0; p = p->next) {
        		if (p->next == handle) {
        			p->next = handle->next;
        		}
        	}
	}

	free(handle);

	return ok > 0 ? 0 : -1;
}

/**
 * Initiates lookup of the given host name to find its IP address.
 *
 * @param hostname the host name for which an ip address is needed,
 *        a null-terminated string
 * @param pAddress base of byte array to receive the address
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an
 *        IPv4 address, 16 if it is an IPv6 address
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there is a network error
 * @retval JAVACALL_INVALID_ARGUMENT    if maxLen is too small to receive the address
 */
javacall_result javacall_network_gethostbyname_start(char *hostname,
    unsigned char *pAddress, int maxLen, /*OUT*/ int *pLen, /*OUT*/ void **pHandle, /*OUT*/ void **pContext) {

    struct in_addr addr;
    (void)pContext;
    int connected = isNetConnected();

    javacall_printf("VITAJ2ME-TRACE: gethostbyname %s\n", hostname);

    if (!connected) {
        /* Vita: la conexión la gestiona el sistema; si no hay red, fallar. */
        return JAVACALL_FAIL;
    }


#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("javacall_network_gethostbyname_start: %s\n", hostname);
#endif

#ifdef BYPASS_CMWAP_GATEWAY
    if (!strcmp(hostname, "10.0.0.172")) {
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_print("CMWAP gateway - 10.0.0.172 is not supported!\n");
#endif
        return JAVACALL_FAIL;
    }
#endif

    if (inet_aton(hostname, &addr) != 0) {
#ifdef DEBUG_JAVACALL_NETWORK
    	javacall_printf("inet_aton ok\n");
#endif
    	if ((int)sizeof(addr) <= maxLen) {
            memcpy(pAddress, &addr.s_addr, sizeof(addr));
            *pLen = sizeof(addr);
    	    return JAVACALL_OK;
    	}
    }
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_printf("inet_aton fail\n");
#endif
    if (JAVACALL_OK == lookup_dns_cache(hostname, pAddress, maxLen, pLen)) {
#ifdef DEBUG_JAVACALL_NETWORK
    	 javacall_printf("dns cache hit\n");
#endif
        return JAVACALL_OK;
    }

#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("dns cache miss\n");
#endif

    *pHandle = start_lookup_ip(hostname, maxLen);
    if (*pHandle) {
        return JAVACALL_WOULD_BLOCK;
    } else {
        return JAVACALL_FAIL;
    }
}

/**
 * Finishes a pending host name lookup operation.
 *
 * @param pAddress base of byte array to receive the address
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an
 *        IPv4 address, 16 if it is an IPv6 address
 * @param handle the handle returned by the gethostbyname_start function
 * @param context the context returned by the gethostbyname_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation;\n
 * @retval JAVACALL_FAIL        if there is a network error
 */
javacall_result javacall_network_gethostbyname_finish(unsigned char *pAddress,
    int maxLen,int *pLen,void *handle,void *context) {
    (void)context;

    if (end_lookup_ip(handle, pAddress, maxLen, pLen) == 0) {
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_printf("javacall_network_gethostbyname_finish ok\n");
#endif
    	 return JAVACALL_OK;
    } else {
#ifdef DEBUG_JAVACALL_NETWORK
        javacall_printf("javacall_network_gethostbyname_finish fail\n");
#endif
        return JAVACALL_FAIL;
    }
}

/**
 * Gets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to get. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param pOptval returns the option's value
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
//pcsl_network_getsockopt
javacall_result /*OPTIONAL*/ javacall_network_getsockopt(javacall_handle handle, javacall_socket_option flag, /*OUT*/ int *pOptval) {

    int level = SOL_SOCKET;
    int optname;
    socklen_t optsize = sizeof(optname);
    struct linger lbuf ;
    void * opttarget = (void *) pOptval ;

    int fd = (int)(long)handle;
    switch (flag) {
    case JAVACALL_SOCK_DELAY: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        break;
    case JAVACALL_SOCK_LINGER: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        break;
    case JAVACALL_SOCK_KEEPALIVE: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;
    case JAVACALL_SOCK_RCVBUF: /* RCVBUF */
        optname = SO_RCVBUF;
        break;
    case JAVACALL_SOCK_SNDBUF: /* SNDBUF */
        optname = SO_SNDBUF;
        break;
    default:
        return JAVACALL_FAIL;
    }

    if (getsockopt(fd, level,  optname, opttarget, &optsize) == 0 ) {
    	 if (optname == TCP_NODELAY) {
    	     if (*pOptval == 0) {
    	         *pOptval = 1;
    	     } else {
    	         *pOptval =0;
    	     }
    	 }

        if (optname == SO_LINGER) {
            /* If linger is on return the number of seconds. */
            *pOptval = (lbuf.l_onoff == 0 ? 0 : lbuf.l_linger) ;
        }
        return JAVACALL_OK;
    } else {
        return JAVACALL_FAIL;
    }
}

/**
 * Sets an option's value for a platform-specific TCP socket or datagram
 *
 * @param handle handle of an open connection
 * @param flag socket option to set. Must be one of the values defined in
 *             <tt>javax.microedition.io.SocketConnection</tt>
 * @param optval the value to be set for this option
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if there was an IO error
 * @retval JAVACALL_INVALID_ARGUMENT    if the platform did not accept the value for the option changed
 */
//pcsl_network_setsockopt
javacall_result /*OPTIONAL*/ javacall_network_setsockopt(javacall_handle handle, javacall_socket_option flag, int optval) {

    int    level = SOL_SOCKET;
    int    optsize =  sizeof(optval);
    int    optname;
    struct linger lbuf ;
    void * opttarget = (void *) & optval ;

    int fd = (int)(long)handle;

#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("javacall_network_setsockopt(flag=0x%x, optval=0x%x)\n",  flag, optval);
#endif

    switch (flag) {
    case JAVACALL_SOCK_DELAY: /* DELAY */
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
        if (optval == 0) {
            optval = 1;
        } else {
            optval = 0;
        }
        break;
    case JAVACALL_SOCK_LINGER: /* LINGER */
        opttarget = (void *) &lbuf ;
        optsize = sizeof (struct linger);
        optname = SO_LINGER;
        if (optval == 0) {
            lbuf.l_onoff = 0;
            lbuf.l_linger = 0;
        } else {
            lbuf.l_onoff = 1;
            lbuf.l_linger = optval;
        }
        break;
    case JAVACALL_SOCK_KEEPALIVE: /* KEEPALIVE */
        optname = SO_KEEPALIVE;
        break;
    case JAVACALL_SOCK_RCVBUF: /* RCVBUF */
        optname = SO_RCVBUF;
        if (optval < 8192) optval = 8192;
        break;
    case JAVACALL_SOCK_SNDBUF: /* SNDBUF */
        optname = SO_SNDBUF;
        if (optval < 8192) optval = 8192;
        break;
    default:
        return JAVACALL_INVALID_ARGUMENT;
    }

    if (setsockopt(fd, level,  optname, opttarget, optsize) != 0) {
#ifdef DEBUG_JAVACALL_NETWORK
    javacall_printf("setsockopt fail.\n");
#endif
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}


///////////////////////////////////////////////////////////////////////

/**
 * Gets the IP address of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocaladdr(
    javacall_handle handle,
    char *pAddress) {
    javacall_print("javacall: not implemented : javacall_socket_getlocaladdr\n");
    return JAVACALL_FAIL;
}

/**
 * Gets the IP address of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pAddress base of byte array to receive the address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getremoteaddr(
    void *handle,
    char *pAddress)
{
    javacall_print("javacall: not implemented! : javacall_socket_getremoteaddr\n");
    return JAVACALL_FAIL;
}

/**
 * Gets the port number of the local socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_getlocalport(javacall_handle handle,int *pPortNumber) {
    javacall_print("javacall: not implemented : javacall_socket_getlocalport\n");
    return JAVACALL_FAIL;
}

/**
 * Gets the port number of the remote socket endpoint.
 *
 * @param handle handle of an open connection
 * @param pPortNumber returns the local port number
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result javacall_socket_getremoteport(
    void *handle,
    int *pPortNumber)
{
    javacall_print("javacall: not implemented : javacall_socket_getremoteport\n");
    return JAVACALL_FAIL;
}

/**
 * Gets the string representation of the local device's IP address.
 * This function returns dotted quad IP address as a string in the
 * output parameter and not the host name.
 *
 * @param pLocalIPAddress base of char array to receive the local
 *        device's IP address
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_get_local_ip_address_as_string(/*OUT*/ char *pLocalIPAddress) {
    javacall_print("javacall: not implemented : javacall_network_get_local_ip_address_as_string\n");
    return JAVACALL_FAIL;
}

/**
 * Translates the given IP address into a host name.
 *
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function to complete the operation
 * @retval JAVACALL_FAIL        if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_gethostbyaddr_start(int ipn,
    char *hostname, /*OUT*/ javacall_handle* pHandle, void **pContext) {

    /*
     * Note there is no spec requirement to look up the the address to
     * to get a hostname. It is not a good idea to perform the lookup,
     * since this function is called EVERY time
     * a datagram is received and on a cell phone network this lookup
     * can take some time and really effect performance for receiving
     * datagrams.
     */
    javacall_print("javacall: not implemented : javacall_network_gethostbyaddr_start\n");
    return JAVACALL_FAIL;
}

/**
 * See javacall_network.h for definition.
 */
javacall_result javacall_server_socket_set_notifier(javacall_handle handle, javacall_bool set) {
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/
int javacall_network_error(void * handle) {
    return 0;
}

/**
 * Finishes a pending host name lookup operation.
 *
 * @param ipn Raw IP address to translate
 * @param hostname the host name. The value of <tt>host</tt> is set by
 *             this function.
 * @param pHandle address of variable to receive the handle to for
 *        unblocking the Java thread; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK.
 * @param context the context returned by the getHostByAddr_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there is a network error
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_network_gethostbyaddr_finish(int ipn,
    char *hostname, /*OUT*/ javacall_handle* pHandle, void *context) {
    javacall_print("javacall: not implemented : javacall_network_gethostbyaddr_finish\n");
    return JAVACALL_FAIL;
}

/**
 * Gets the http / https proxy address. This method is
 * called when the <tt>com.sun.midp.io.http.proxy</tt> or
 * <tt>com.sun.midp.io.https.proxy</tt> internal property
 * is retrieved.
 *
 * @param pHttpProxy base of char array to receive the hostname followed
 *          by ':' and port. - ex) webcache.thecompany.com:8080.
 *          Size of the pHttpProxy should be (MAX_HOST_LENGTH + 6).
 * @param pHttpsProxy base of char array to receive the hostname followed
 *          by ':' and port. - ex) webcache.thecompany.com:8080.
 *          Size of the pHttpsProxy should be (MAX_HOST_LENGTH + 6).
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there is a network error
 */
javacall_result /*OPTIONAL*/ javacall_network_get_http_proxy(/*OUT*/ char *pHttpProxy, /*OUT*/ char *pHttpsProxy) {
    /* Vita: sin API de proxy por-perfil para apps; conexión directa. */
    javacall_print("javacall_network_get_http_proxy: no proxy en Vita\n");
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_network_get_local_host_name(/*OUT*/ char *pLocalHost) {
    javacall_print("javacall: not implemented : javacall_network_get_local_host_name \n");
    return JAVACALL_FAIL;
}

/**
    ??????????????????? undefined
 */
char* javacall_inet_ntoa(void *address) {
    javacall_print("javacall: not implemented : javacall_inet_ntoa\n");
    return NULL; //win32: return inet_ntoa(*((struct in_addr*)address));
}

/** @} */
#ifdef __cplusplus
}
#endif
