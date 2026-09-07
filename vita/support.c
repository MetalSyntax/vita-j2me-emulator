// vita/support.c — Soporte del frontend Fase 2b.
//
// - mk_dir/rm_dir: los espera fileconnection.c (en PSP vivían en
//   psp/pspkvm.c). Wrappers finos de SceIo.
// - stricmp: newlib de Vita no lo trae (solo strcasecmp); lo usan
//   fuentes MIDP/JSR portadas tal cual.

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <strings.h>
#include <sys/stat.h>

typedef int SceMode;

int mk_dir(const char *dir, SceMode mode) {
    int rc = sceIoMkdir(dir, (SceMode)mode);
    if (rc < 0) {
        /* Si ya existe no es error: se comprueba abriéndolo. */
        SceUID dfd = sceIoDopen(dir);
        if (dfd >= 0) {
            sceIoDclose(dfd);
            return 0;
        }
    }
    return rc;
}

int rm_dir(const char *dir) {
    return sceIoRmdir(dir);
}

int stricmp(const char *a, const char *b) {
    return strcasecmp(a, b);
}
