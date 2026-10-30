/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */ 


#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_multimedia.h"

/* =====================================================================
 * VitaJ2ME audio core (Fase 3).
 * Backend: SceAudioOut directo (puerto MAIN 48kHz estéreo S16; si está
 * ocupado se usa BGM como fallback para permitir 2 sonidos solapados).
 *  - WAV/PCM: buffering Java -> parse RIFF -> worker thread por player.
 *  - Tonos Manager (audio/x-tone-seq y play_tone): seno sintetizado.
 *  - MIDI (sesión 45): TinySoundFont (tsf.h/tml.h, MIT) con el banco GM
 *    ux0:data/vitaj2me/lib/gm.sf2, sintetizado en streaming en el hilo
 *    del reproductor (nada de renderizar la canción entera de golpe).
 *  - MP3 (sesión 48): libmpg123 de VitaSDK en modo feed, decodificado en
 *    streaming a 48 kHz estéreo (MPG123_FORCE_RATE) en el mismo hilo.
 *  - AAC/AMR/vídeo/grabación: NO implementados (FAIL seguro).
 * ===================================================================== */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"
#include <mpg123.h>

#define VITA_AUDIO_FREQ   48000
#define VITA_AUDIO_GRAIN  1024   /* samples por sceAudioOutOutput */
#define VITA_PLAYER_WAV   1
#define VITA_PLAYER_MIDI  2
#define VITA_PLAYER_MP3   3
#define VITA_GM_BANK      "ux0:data/vitaj2me/lib/gm.sf2"
#define VITA_MIDI_BLOCK   64     /* resolución temporal de eventos MIDI */

typedef struct {
    javacall_int64 playerId;
    int kind;
    unsigned char *data;   /* bytes crudos acumulados vía do_buffering */
    long dataLen;
    long dataCap;
    short *pcm;            /* PCM S16 estéreo a 48kHz, parseado */
    long frames;
    long posFrames;
    int volume;            /* 0..100 */
    int muted;
    int state;             /* 0 idle, 1 playing, 2 paused */
    int port;              /* puerto SceAudioOut o -1 */
    int useBgm;            /* 1 si se cayó a BGM */
    SceUID thread;
    int threadRun;
    SceUID lock;
    /* MIDI */
    tsf *synth;
    tml_message *midi, *midiCur;
    double midiMs;
    unsigned int midiLenMs;
    /* MP3 */
    mpg123_handle *mp3;
    long mp3Frames;        /* frames de 48 kHz ya emitidos */
    short grain[VITA_AUDIO_GRAIN * 2]; /* propio: antes era static compartido */
} vita_player_t;

static int vita_audio_ready = 0;

/* Menú del juego abierto (vita/launcher.c): los reproductores callan. */
volatile int vita_media_paused = 0;

/* Banco GM compartido; cada reproductor MIDI usa una copia ligera
 * (tsf_copy comparte las muestras). */
static tsf *vita_gm_bank = NULL;
static int vita_gm_tried = 0;

static tsf *vita_gm_get(void) {
    if (!vita_gm_tried) {
        vita_gm_tried = 1;
        vita_gm_bank = tsf_load_filename(VITA_GM_BANK);
        javacall_printf("VITAJ2ME-MIDI: banco %s %s\n", VITA_GM_BANK,
                        vita_gm_bank ? "cargado" : "NO encontrado (MIDI mudo)");
    }
    return vita_gm_bank;
}

static void vita_midi_rewind(vita_player_t *p) {
    p->midiCur = p->midi;
    p->midiMs = 0;
    tsf_reset(p->synth);
    tsf_channel_set_bank_preset(p->synth, 9, 128, 0); /* percusión GM */
}

static void vita_midi_apply(tsf *f, const tml_message *m, int notes) {
    switch (m->type) {
    case TML_PROGRAM_CHANGE:
        tsf_channel_set_presetnumber(f, m->channel, m->program,
                                     m->channel == 9);
        break;
    case TML_NOTE_ON:
        if (notes) tsf_channel_note_on(f, m->channel, m->key,
                                       m->velocity / 127.0f);
        break;
    case TML_NOTE_OFF:
        if (notes) tsf_channel_note_off(f, m->channel, m->key);
        break;
    case TML_PITCH_BEND:
        tsf_channel_set_pitchwheel(f, m->channel, m->pitch_bend);
        break;
    case TML_CONTROL_CHANGE:
        tsf_channel_midi_control(f, m->channel, m->control, m->control_value);
        break;
    }
}

/* Parsea el .mid recibido por buffering y prepara el sintetizador. */
static int vita_midi_prepare(vita_player_t *p) {
    tsf *bank = vita_gm_get();
    if (!bank) return -1;
    if (!p->midi) {
        p->midi = tml_load_memory(p->data, (int)p->dataLen);
        if (!p->midi) return -1;
        tml_get_info(p->midi, NULL, NULL, NULL, NULL, &p->midiLenMs);
    }
    if (!p->synth) {
        p->synth = tsf_copy(bank);
        if (!p->synth) return -1;
        tsf_set_output(p->synth, TSF_STEREO_INTERLEAVED, VITA_AUDIO_FREQ, 0);
        tsf_set_max_voices(p->synth, 48);
        vita_midi_rewind(p);
    }
    return 0;
}

static void vita_mp3_close(vita_player_t *p) {
    if (p->mp3) {
        mpg123_delete(p->mp3);
        p->mp3 = NULL;
    }
    p->mp3Frames = 0;
}

/* Decodificador en modo feed con todo el fichero ya entregado: el
 * buffering de Java trae el .mp3 completo. Rebobinar = recrearlo. */
static int vita_mp3_prepare(vita_player_t *p) {
    static int inited = 0;
    int err = 0;
    if (p->mp3) return 0;
    if (!inited) {
        mpg123_init();
        inited = 1;
    }
    p->mp3 = mpg123_new(NULL, &err);
    if (!p->mp3) return -1;
    mpg123_param(p->mp3, MPG123_FLAGS, MPG123_FORCE_STEREO | MPG123_QUIET, 0);
    mpg123_param(p->mp3, MPG123_FORCE_RATE, VITA_AUDIO_FREQ, 0);
    mpg123_format_none(p->mp3);
    if (mpg123_format(p->mp3, VITA_AUDIO_FREQ, MPG123_STEREO,
                      MPG123_ENC_SIGNED_16) != MPG123_OK ||
        mpg123_open_feed(p->mp3) != MPG123_OK ||
        mpg123_feed(p->mp3, p->data, (size_t)p->dataLen) != MPG123_OK) {
        javacall_printf("VITAJ2ME-MP3: no se pudo abrir (%s)\n",
                        mpg123_strerror(p->mp3));
        vita_mp3_close(p);
        return -1;
    }
    p->mp3Frames = 0;
    return 0;
}

/* Rinde un grano; 1 = fin del fichero. */
static int vita_mp3_render(vita_player_t *p) {
    unsigned char *out = (unsigned char *)p->grain;
    size_t want = sizeof(p->grain), got = 0;
    int tries = 0;
    while (got < want && tries++ < 64) {
        size_t done = 0;
        int r = mpg123_read(p->mp3, out + got, want - got, &done);
        got += done;
        if (r == MPG123_NEW_FORMAT || (r == MPG123_OK && done)) continue;
        if (r != MPG123_OK) break;  /* NEED_MORE/DONE/ERR: no queda nada */
    }
    if (got < want) memset(out + got, 0, want - got);
    p->mp3Frames += (long)(got / 4);
    return got == 0;
}

/* Rinde un grano; devuelve 1 cuando la canción (y su cola) terminó. */
static int vita_midi_render(vita_player_t *p) {
    int i;
    for (i = 0; i < VITA_AUDIO_GRAIN; i += VITA_MIDI_BLOCK) {
        p->midiMs += VITA_MIDI_BLOCK * 1000.0 / VITA_AUDIO_FREQ;
        while (p->midiCur && p->midiCur->time <= p->midiMs) {
            vita_midi_apply(p->synth, p->midiCur, 1);
            p->midiCur = p->midiCur->next;
        }
        tsf_render_short(p->synth, p->grain + i * 2, VITA_MIDI_BLOCK, 0);
    }
    return !p->midiCur && (p->midiMs > p->midiLenMs + 1000 ||
                           !tsf_active_voice_count(p->synth));
}

static void vita_player_lock(vita_player_t *p) {
    if (p->lock >= 0) sceKernelWaitSema(p->lock, 1, NULL);
}
static void vita_player_unlock(vita_player_t *p) {
    if (p->lock >= 0) sceKernelSignalSema(p->lock, 1);
}

static void vita_player_apply_volume(vita_player_t *p) {
    if (p->port < 0) return;
    int v = p->muted ? 0 : (p->volume * SCE_AUDIO_OUT_MAX_VOL) / 100;
    int vol[2] = { v, v };
    sceAudioOutSetVolume(p->port,
        SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
}

static int vita_player_open_port(vita_player_t *p) {
    if (p->port >= 0) return 0;
    p->port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                                  VITA_AUDIO_GRAIN, VITA_AUDIO_FREQ,
                                  SCE_AUDIO_OUT_MODE_STEREO);
    p->useBgm = 0;
    if (p->port < 0) {
        p->port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,
                                      VITA_AUDIO_GRAIN, VITA_AUDIO_FREQ,
                                      SCE_AUDIO_OUT_MODE_STEREO);
        p->useBgm = 1;
    }
    if (p->port < 0) {
        p->port = -1;
        return -1;
    }
    vita_player_apply_volume(p);
    return 0;
}

static void vita_player_close_port(vita_player_t *p) {
    if (p->port >= 0) {
        sceAudioOutReleasePort(p->port);
        p->port = -1;
    }
}

/* Parse mínimo RIFF/WAV -> PCM S16 estéreo 48kHz (re-muestreo nearest). */
static int vita_parse_wav(vita_player_t *p) {
    unsigned char *d = p->data;
    long n = p->dataLen;
    if (n < 44 || memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) {
        return -1;
    }
    long off = 12;
    int channels = 0, rate = 0, bits = 0;
    long dataOff = -1, dataLen = 0;
    while (off + 8 <= n) {
        long chunkLen = (long)(d[off+4] | (d[off+5]<<8) | (d[off+6]<<16) | (d[off+7]<<24));
        if (memcmp(d + off, "fmt ", 4) == 0 && chunkLen >= 16) {
            int audioFmt = d[off+8] | (d[off+9]<<8);
            channels = d[off+10] | (d[off+11]<<8);
            rate = d[off+12] | (d[off+13]<<8) | (d[off+14]<<16) | (d[off+15]<<24);
            bits = d[off+24] | (d[off+25]<<8);
            if (audioFmt != 1) return -1; /* solo PCM */
        } else if (memcmp(d + off, "data", 4) == 0) {
            dataOff = off + 8;
            dataLen = chunkLen;
            break; /* data suele ir al final */
        }
        off += 8 + chunkLen + (chunkLen & 1);
    }
    if (dataOff < 0 || channels < 1 || channels > 2 || (bits != 8 && bits != 16)) {
        return -1;
    }
    if (dataOff + dataLen > n) dataLen = n - dataOff;
    long inFrames = dataLen / (channels * (bits / 8));
    if (inFrames <= 0) return -1;
    /* Re-muestreo a 48kHz: outFrames = inFrames * 48000 / rate */
    long outFrames = (long)((double)inFrames * VITA_AUDIO_FREQ / rate) + 1;
    short *pcm = (short *)malloc(outFrames * 2 * sizeof(short));
    if (!pcm) return -1;
    long i;
    for (i = 0; i < outFrames; i++) {
        long si = (long)((double)i * rate / VITA_AUDIO_FREQ);
        if (si >= inFrames) si = inFrames - 1;
        long vL, vR;
        if (bits == 16) {
            short *s = (short *)(d + dataOff + si * channels * 2);
            vL = s[0];
            vR = (channels > 1) ? s[1] : s[0];
        } else {
            unsigned char *u = d + dataOff + si * channels;
            vL = ((int)u[0] - 128) * 256;
            vR = (channels > 1) ? (((int)u[1] - 128) * 256) : vL;
        }
        pcm[i * 2] = (short)vL;
        pcm[i * 2 + 1] = (short)vR;
    }
    free(p->pcm);
    p->pcm = pcm;
    p->frames = outFrames;
    p->posFrames = 0;
    return 0;
}

static int vita_player_thread(SceSize args, void *argp) {
    (void)args;
    vita_player_t *p = *(vita_player_t **)argp;
    short *grain = p->grain;
    while (p->threadRun) {
        vita_player_lock(p);
        int playing = p->state == 1 && !vita_media_paused &&
            (p->kind == VITA_PLAYER_MIDI ? p->synth != NULL
             : p->kind == VITA_PLAYER_MP3 ? p->mp3 != NULL
                                          : (p->pcm && p->posFrames < p->frames));
        vita_player_unlock(p);
        if (!playing) {
            sceKernelDelayThreadCB(10000); /* 10ms */
            continue;
        }
        if (p->kind == VITA_PLAYER_MIDI || p->kind == VITA_PLAYER_MP3) {
            int ended;
            vita_player_lock(p);
            if (p->kind == VITA_PLAYER_MIDI) {
                ended = vita_midi_render(p);
                if (ended) vita_midi_rewind(p);
            } else {
                ended = vita_mp3_render(p);
                if (ended) vita_mp3_close(p);
            }
            if (ended) p->state = 0;
            vita_player_unlock(p);
            sceAudioOutOutput(p->port, grain);
            if (ended) {
                javanotify_on_media_notification(
                    JAVACALL_EVENT_MEDIA_END_OF_MEDIA, p->playerId, 0);
            }
            continue;
        }
        long left = p->frames - p->posFrames;
        long chunk = left > VITA_AUDIO_GRAIN ? VITA_AUDIO_GRAIN : left;
        memcpy(grain, p->pcm + p->posFrames * 2, (size_t)chunk * 2 * sizeof(short));
        if (chunk < VITA_AUDIO_GRAIN) {
            memset(grain + chunk * 2, 0,
                   (size_t)(VITA_AUDIO_GRAIN - chunk) * 2 * sizeof(short));
        }
        sceAudioOutOutput(p->port, grain);
        vita_player_lock(p);
        p->posFrames += chunk;
        int ended = (p->posFrames >= p->frames);
        if (ended) {
            p->state = 0;
            p->posFrames = 0;
        }
        vita_player_unlock(p);
        if (ended) {
            javanotify_on_media_notification(
                JAVACALL_EVENT_MEDIA_END_OF_MEDIA, p->playerId, 0);
        }
    }
    return 0;
}

/* --- Sintetizador de tonos (Manager.playTone / audio/x-tone-seq) --- */
static volatile int vita_tone_abort = 0;

static double vita_note_freq(long note) {
    /* note = ln(freq/8.176) * 17.31234049066755  =>  freq = 8.176 * e^(n/17.31) */
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    return 8.176 * exp((double)note / 17.31234049066755);
}


/**
 * Get multimedia capabilities of the device.
 * This function should return pointer to static array of javacall_media_caps value
 * The last item of javacall_media_caps array should hold NULL mimeType value
 * Java layer will use this NULL value as a end of item mark
 */
static const javacall_media_caps vita_media_caps[] = {
    /* Fin de lista: mimeType NULL (lo usa Java como marca de fin). */
    {NULL, 3, {"device", "capture", "http"}}
};

const javacall_media_caps* javacall_media_get_caps(void) {
	return vita_media_caps;
}

/**
 * Query whether audio mixing is supported or not
 */
javacall_bool javacall_media_supports_mixing() {
    return JAVACALL_TRUE; /* MAIN + fallback BGM */
}

/**
 * Call this function when VM starts
 * Perform global initialization operation
 *
 * @retval JOK      success
 * @retval JFAIL    fail
 */
javacall_result javacall_media_initialize(void) {
    vita_audio_ready = 1;
    return JAVACALL_OK;
}


/**
 * Call this function when VM ends
 * Perfrom global free operaiton
 *
 * @retval JOK      success
 * @retval JFAIL    fail
 */
javacall_result javacall_media_finalize(void) {
    vita_audio_ready = 0;
    vita_tone_abort = 1;
    return JAVACALL_OK;
}

/**
 * Java MMAPI call this function to create native media handler.
 * This function is called at the first time to initialize native library.
 * You can do your own initializatio job from this function.
 * 
 * @param playerId      Unique player object ID for this playing
 *                      This unique ID is generated by Java MMAPI library.
 * @param mime          Mime unicode string
 * @param mimeLength    String length of mimeType
 * @param uri           URI unicode string to media data
 * @param uriLength     String length of URI
 * @param contentLength Content length in bytes
 *                      If Java MMAPI couldn't determine contenet length, this value should be -1
 * 
 * @return              Handle of native library. if fail return NULL.
 */
javacall_handle javacall_media_create(javacall_int64 playerId,
                                      const javacall_utf16* mime,
                                      long mimeLength,
                                      const javacall_utf16* uri,
                                      long uriLength,
                                      long contentLength)
{
    char mimeBuf[64];
    long i;
    (void)uri; (void)uriLength; (void)contentLength;

    if (!vita_audio_ready) {
        javacall_media_initialize();
    }
    if (mimeLength <= 0 || mimeLength >= (long)sizeof(mimeBuf)) {
        return 0;
    }
    for (i = 0; i < mimeLength; i++) {
        mimeBuf[i] = (char)mime[i];
    }
    mimeBuf[mimeLength] = '\0';

    /* WAV y MIDI por buffering. MP3/AAC/tonos-secuencia devuelven NULL
     * (Java lanza MediaException controlada). */
    int kind = 0;
    if (!strcmp(mimeBuf, JAVACALL_AUDIO_WAV_MIME) ||
        !strcmp(mimeBuf, JAVACALL_AUDIO_WAV_MIME_2)) {
        kind = VITA_PLAYER_WAV;
    } else if (!strcmp(mimeBuf, JAVACALL_AUDIO_MIDI_MIME) ||
               !strcmp(mimeBuf, JAVACALL_AUDIO_MIDI_MIME_2) ||
               !strcmp(mimeBuf, JAVACALL_AUDIO_SP_MIDI_MIME) ||
               !strcmp(mimeBuf, "audio/x-midi")) {
        if (!vita_gm_get()) return 0;
        kind = VITA_PLAYER_MIDI;
    } else if (!strcmp(mimeBuf, JAVACALL_AUDIO_MP3_MIME) ||
               !strcmp(mimeBuf, JAVACALL_AUDIO_MP3_MIME_2) ||
               !strcmp(mimeBuf, "audio/mpeg3") ||
               !strcmp(mimeBuf, "audio/x-mp3")) {
        kind = VITA_PLAYER_MP3;
    } else {
        javacall_printf("javacall_media_create: mime no soportado: %s\n", mimeBuf);
        return 0;
    }

    vita_player_t *p = (vita_player_t *)calloc(1, sizeof(vita_player_t));
    if (!p) {
        return 0;
    }
    p->playerId = playerId;
    p->kind = kind;
    p->port = -1;
    p->volume = 100;
    p->lock = sceKernelCreateSema("VitaPlayerMutex", 0, 1, 1, NULL);
    p->threadRun = 1;
    p->thread = sceKernelCreateThread("vitaj2me_audio", vita_player_thread,
                                      0x40, 0x4000, 0, 0, 0);
    if (p->thread >= 0) {
        sceKernelStartThread(p->thread, sizeof(&p), &p);
    }
    return (javacall_handle)p;
}

/**
 * Java MMAPI call this function to create native media handler.
 * This function is called at the first time to initialize native library.
 * You can do your own initializatio job from this function.
 * 
 * @param playerId      Unique player object ID for this playing
 * @param mediaType     Media content type
 * @param fileName      File name to play (device's native path format)
 * @return              Handle of native library. if fail return NULL.
 */
javacall_handle javacall_media_create2(int playerId, javacall_media_type mediaType, const char* fileName) {
    return 0;
}

/**
 * Close native media player that created by creat or creat2 API call
 * After this call, you can't use any other function in this library
 * 
 * @param handle  Handle to the library.
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_media_close(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    p->state = 0;
    vita_player_close_port(p);
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * finally destroy native media player previously closed by
 * javacall_media_close. intended to be used by finalizer
 * 
 * @param handle  Handle to the library.
 * 
 * @retval JAVACALL_OK      Java VM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    Java VM will raise the media exception
 */
javacall_result javacall_media_destroy(javacall_handle handle){
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    p->threadRun = 0;
    if (p->thread >= 0) {
        sceKernelWaitThreadEnd(p->thread, NULL, NULL);
        sceKernelDeleteThread(p->thread);
    }
    vita_player_close_port(p);
    if (p->synth) tsf_close(p->synth);
    vita_mp3_close(p);
    if (p->midi) tml_free(p->midi);
    free(p->data);
    free(p->pcm);
    if (p->lock >= 0) {
        sceKernelDeleteSema(p->lock);
    }
    free(p);
    return JAVACALL_OK;
}


/**
 * Request to acquire device resources used to play media data.
 * You could implement this function to control device resource usage.
 * If there is no valid device resource to play media data, return JAVACALL_FAIL.
 * 
 * @param handle  Handle to the library.
 * @retval JAVACALL_OK      Can acquire device
 * @retval JAVACALL_FAIL    There is no valid device resource
 */
javacall_result javacall_media_acquire_device(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    /* El puerto se abre de forma perezosa en start(); aquí basta existir. */
    return JAVACALL_OK;
}

/**
 * Release device resource. 
 * Java MMAPI call this function to release limited device resources.
 * 
 * @param handle  Handle to the library.
 * @retval JAVACALL_OK      Can release device
 * @retval JAVACALL_FAIL    Can't release device
 */
javacall_result javacall_media_release_device(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    p->state = 0;
    vita_player_close_port(p);
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Ask to the native layer.
 * Is this protocol handled by native layer or Java layer?
 * 
 * @retval JAVACALL_OK      Yes, this protocol handled by device.
 * @retval JAVACALL_FAIL    No, please handle this protocol from Java.
 */
javacall_result javacall_media_protocol_handled_by_device(javacall_handle handle) {
    (void)handle;
    /* Java hace buffering vía do_buffering (file/http/capture). */
    return JAVACALL_FAIL;
}

/**
 * Java MMAPI call this function to send media data to this library
 * This function can be called multiple time to send large media data
 * Native library could implement buffering by using any method (for example: file, heap and etc...)
 * And, buffering occured in sequentially. not randomly.
 * 
 * @param handle    Handle to the library
 * @param buffer    Media data buffer pointer
 * @param length    Length of media data
 * @param offset    Offset. If offset value is 0, it means start of buffering
 *                  It'll be incremented as buffering progress
 *                  You can determine your internal buffer's writting position by using this value
 * @return          If success return 'length of buffered data' else return -1
 */
long javacall_media_do_buffering(javacall_handle handle, const void* buffer, long length, long offset) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p || !buffer || length <= 0 || offset < 0) {
        return -1;
    }
    vita_player_lock(p);
    if (offset == 0) {
        p->dataLen = 0; /* nuevo stream */
        if (p->midi) { tml_free(p->midi); p->midi = p->midiCur = NULL; }
        vita_mp3_close(p);
        free(p->pcm);
        p->pcm = NULL;
        p->frames = 0;
        p->posFrames = 0;
    }
    if (offset != p->dataLen) {
        /* Solo buffering secuencial. */
        vita_player_unlock(p);
        return -1;
    }
    if (p->dataLen + length > p->dataCap) {
        long newCap = p->dataLen + length + 65536;
        unsigned char *nd = (unsigned char *)realloc(p->data, (size_t)newCap);
        if (!nd) {
            vita_player_unlock(p);
            return -1;
        }
        p->data = nd;
        p->dataCap = newCap;
    }
    memcpy(p->data + p->dataLen, buffer, (size_t)length);
    p->dataLen += length;
    vita_player_unlock(p);
    return p->dataLen;
}

/**
 * MMAPI call this function to clear(delete) buffered media data
 * You have to clear any resources created from previous buffering
 * 
 * @param handle    Handle to the library
 * @retval JAVACALL_OK      Can clear buffer
 * @retval JAVACALL_FAIL    Can't clear buffer
 */
javacall_result javacall_media_clear_buffer(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    p->state = 0;
    p->dataLen = 0;
    free(p->pcm);
    p->pcm = NULL;
    p->frames = 0;
    p->posFrames = 0;
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Start media playing
 * 
 * @param handle    Handle to the library
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_start(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    if (p->kind == VITA_PLAYER_MIDI) {
        if (vita_midi_prepare(p) != 0) {
            vita_player_unlock(p);
            javacall_printf("javacall_media_start: MIDI inválido\n");
            return JAVACALL_FAIL;
        }
    } else if (p->kind == VITA_PLAYER_MP3) {
        if (vita_mp3_prepare(p) != 0) {
            vita_player_unlock(p);
            return JAVACALL_FAIL;
        }
    } else if (!p->pcm) {
        if (vita_parse_wav(p) != 0) {
            vita_player_unlock(p);
            javacall_printf("javacall_media_start: WAV inválido\n");
            return JAVACALL_FAIL;
        }
    }
    if (vita_player_open_port(p) != 0) {
        vita_player_unlock(p);
        javacall_printf("javacall_media_start: sin puerto de audio\n");
        return JAVACALL_FAIL;
    }
    p->state = 1;
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Stop media playing
 * 
 * @param handle      Handle to the library
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_stop(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    p->state = 0;
    p->posFrames = 0;
    if (p->synth) vita_midi_rewind(p);
    vita_mp3_close(p);
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Pause media playing
 * 
 * @param handle      Handle to the library
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_pause(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    if (p->state == 1) {
        p->state = 2;
    }
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Resume media playing
 * 
 * @param handle      Handle to the library
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_resume(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    if (p->state == 2) {
        p->state = 1;
    }
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Get current playing media time
 * 
 * @param handle      Handle to the library
 * @return          If success return time in ms else return -1
 */
long javacall_media_get_time(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (p && p->kind == VITA_PLAYER_MIDI) {
        return p->midi ? (long)p->midiMs : -1;
    }
    if (p && p->kind == VITA_PLAYER_MP3) {
        return (long)(p->mp3Frames * 1000 / VITA_AUDIO_FREQ);
    }
    if (!p || !p->pcm) {
        return -1;
    }
    return (long)(p->posFrames * 1000 / VITA_AUDIO_FREQ);
}

/**
 * Seek to specified time
 * 
 * @param handle      Handle to the library
 * @param ms        Seek position as ms time
 * @return          If success return time in ms else return -1
 */
long javacall_media_set_time(javacall_handle handle, long ms) {
    vita_player_t *p = (vita_player_t *)handle;
    if (p && p->kind == VITA_PLAYER_MIDI && ms >= 0) {
        vita_player_lock(p);
        if (vita_midi_prepare(p) == 0) {
            /* rebobinar y avanzar sin sonar: solo programa/controles */
            vita_midi_rewind(p);
            while (p->midiCur && p->midiCur->time < (unsigned int)ms) {
                vita_midi_apply(p->synth, p->midiCur, 0);
                p->midiCur = p->midiCur->next;
            }
            p->midiMs = ms;
        }
        vita_player_unlock(p);
        return ms;
    }
    if (p && p->kind == VITA_PLAYER_MP3) {
        if (ms != 0) return -1;  /* solo rebobinar (lo único que usan) */
        vita_player_lock(p);
        vita_mp3_close(p);
        if (p->state == 1) vita_mp3_prepare(p);
        vita_player_unlock(p);
        return 0;
    }
    if (!p || !p->pcm || ms < 0) {
        return -1;
    }
    vita_player_lock(p);
    p->posFrames = ms * VITA_AUDIO_FREQ / 1000;
    if (p->posFrames > p->frames) {
        p->posFrames = p->frames;
    }
    vita_player_unlock(p);
    return ms;
}
 
/**
 * Get whole media time in ms
 * 
 * @param handle      Handle to the library
 * @return          If success return time in ms else return -1
 */
long javacall_media_get_duration(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (p && p->kind == VITA_PLAYER_MIDI) {
        if (!p->midi && p->dataLen > 0) {
            vita_player_lock(p);
            vita_midi_prepare(p);
            vita_player_unlock(p);
        }
        return p->midi ? (long)p->midiLenMs : -1;
    }
    if (!p || !p->pcm) {
        return -1;
    }
    return (long)(p->frames * 1000 / VITA_AUDIO_FREQ);
}

/**
 * Get current audio volume
 * Audio volume range have to be in 0 to 100 inclusive
 * 
 * @param mediaType Media type. 
 *                  It could be used to determine H/W audio path for specific audio data type.
 * @return          Volume value
 */
long javacall_media_get_volume(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return 100;
    }
    return p->muted ? 0 : p->volume;
}

/**
 * Set audio volume
 * Audio volume range have to be in 0 to 100 inclusive
 * 
 * @param mediaType Media type. 
 *                  It could be used to determine H/W audio path for specific data type.
 * @param level     Volume value
 * @return          if success return volume level else return -1
 */
long javacall_media_set_volume(javacall_handle handle, long level) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return -1;
    }
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    vita_player_lock(p);
    p->volume = (int)level;
    vita_player_apply_volume(p);
    vita_player_unlock(p);
    return level;
}

/**
 * Is audio muted now?
 * 
 * @param mediaType Media type. 
 *                  It could be used to determine H/W audio path for specific data type.
 * @retval JAVACALL_TRUE    Now in mute state
 * @retval JAVACALL_FALSE   Now in un-mute state
 */
javacall_bool javacall_media_is_mute(javacall_handle handle) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FALSE;
    }
    return p->muted ? JAVACALL_TRUE : JAVACALL_FALSE;
}

/**
 * Mute, Unmute audio
 * 
 * @param mediaType Media type. 
 *                  It could be used to determine H/W audio path for specific data type.
 * @param mute      JAVACALL_TRUE to mute, JAVACALL_FALSE to unmute
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */ 
javacall_result javacall_media_set_mute(javacall_handle handle, javacall_bool mute) {
    vita_player_t *p = (vita_player_t *)handle;
    if (!p) {
        return JAVACALL_FAIL;
    }
    vita_player_lock(p);
    p->muted = (mute == JAVACALL_TRUE) ? 1 : 0;
    vita_player_apply_volume(p);
    vita_player_unlock(p);
    return JAVACALL_OK;
}

/**
 * Turn on or off video rendering alpha channel.
 * If this is on OEM native layer video renderer SHOULD use this mask color
 * and draw on only the region that is filled with this color value.
 * 
 * @param on    Alpha channel is on?
 * @param color Color of alpha channel
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_alpha(javacall_bool on, javacall_pixel color) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get original video width
 * 
 * @param handle      Handle to the library 
 * @param width     Pointer to int variable to get width of video
 *                  Caller can assign NULL value if you don't care about width value.
 * @param height    Pointer to int variable to get height of video
 *                  Caller can assign NULL value if youe don't care about height value.
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_size(javacall_handle handle, 
                                              /*OUT*/ long* width, /*OUT*/ long* height) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Set video rendering position in physical screen
 * 
 * @param handle    Handle to the library 
 * @param x         X position of rendering in pixels
 * @param y         Y position of rendering in pixels
 * @param w         Width of rendering
 * @param h         Height of rendering
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Set video preview visible state to show or hide
 * 
 * @param handle    Handle to the library
 * @param visible   JAVACALL_TRUE to show or JAVACALL_FALSE to hide
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_visible(javacall_handle handle, javacall_bool visible)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Start get current snapshot of video data
 * When snapshot operation doen, call callback function to provide snapshot image data to Java.
 *
 * @param handle            Handle to the library
 * @param imageType         Snapshot image type format as unicode string. For example, "jpeg", "png" or "rgb565".
 * @param length            imageType unicode string length
 * 
 * @retval JAVACALL_OK          Success.
 * @retval JAVACALL_WOULD_BLOCK This operation could takes long time. 
 *                              After this operation finished, MUST send - IMPL_NOTE: EVENT NAME - by using 
 *                              "javanotify_on_media_notification" function call
 * @retval JAVACALL_FAIL        Fail. Invalid encodingFormat or some errors.
 */
javacall_result javacall_media_start_video_snapshot(javacall_handle handle, 
                                                    const javacall_utf16* imageType, long length)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get snapshot data size
 * 
 * @param handle    Handle to the library
 * @param size      Size of snapshot data
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_snapshot_data_size(javacall_handle handle, /*OUT*/ long* size)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get snapshot data
 * 
 * @param handle    Handle to the library
 * @param buffer    Buffer will contains the snapshot data
 * @param size      Size of snapshot data
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_snapshot_data(javacall_handle handle, /*OUT*/ char* buffer, long size)
{
    return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * play simple tone
 *
 * @param note     the note to be played. From 0 to 127 inclusive.
 *                 The frequency of the note can be calculated from the following formula:
 *                    SEMITONE_CONST = 17.31234049066755 = 1/(ln(2^(1/12)))
 *                    note = ln(freq/8.176)*SEMITONE_CONST
 *                    The musical note A = MIDI note 69 (0x45) = 440 Hz.
 * @param duration the duration of the note in ms 
 * @param volume   volume of this play. From 0 to 100 inclusive.
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_play_tone(long note, long duration, long volume) {
    /* Tono Manager síncrono: seno a 48kHz por el puerto MAIN.
     * Bloquea al llamador (normalmente el hilo de tonos de Java). */
    double freq = vita_note_freq(note);
    if (duration <= 0) {
        return JAVACALL_FAIL;
    }
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    if (duration > 5000) duration = 5000; /* seguridad */

    int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                                   VITA_AUDIO_GRAIN, VITA_AUDIO_FREQ,
                                   SCE_AUDIO_OUT_MODE_STEREO);
    if (port < 0) {
        port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,
                                   VITA_AUDIO_GRAIN, VITA_AUDIO_FREQ,
                                   SCE_AUDIO_OUT_MODE_STEREO);
    }
    if (port < 0) {
        return JAVACALL_FAIL;
    }
    int v = (int)(volume * SCE_AUDIO_OUT_MAX_VOL) / 100;
    int vol[2] = { v, v };
    sceAudioOutSetVolume(port,
        SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);

    static short grain[VITA_AUDIO_GRAIN * 2];
    long total = (long)(VITA_AUDIO_FREQ * duration / 1000);
    long done = 0;
    double phase = 0.0;
    double step = 2.0 * 3.141592653589793 * freq / VITA_AUDIO_FREQ;
    vita_tone_abort = 0;
    while (done < total && !vita_tone_abort) {
        long chunk = total - done;
        if (chunk > VITA_AUDIO_GRAIN) chunk = VITA_AUDIO_GRAIN;
        long i;
        for (i = 0; i < chunk; i++) {
            /* Envolvente: ataque/salida de 5ms para evitar clicks. */
            long left = total - done - i;
            double env = 1.0;
            long ramp = VITA_AUDIO_FREQ * 5 / 1000;
            if (done + i < ramp) env = (double)(done + i) / ramp;
            else if (left < ramp) env = (double)left / ramp;
            short s = (short)(sin(phase) * 30000.0 * env);
            phase += step;
            if (phase > 2.0 * 3.141592653589793) phase -= 2.0 * 3.141592653589793;
            grain[i * 2] = s;
            grain[i * 2 + 1] = s;
        }
        if (chunk < VITA_AUDIO_GRAIN) {
            memset(grain + chunk * 2, 0,
                   (size_t)(VITA_AUDIO_GRAIN - chunk) * 2 * sizeof(short));
        }
        sceAudioOutOutput(port, grain);
        done += chunk;
    }
    sceAudioOutReleasePort(port);
    return JAVACALL_OK;
}

/**
 * stop simple tone
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_stop_tone(void) {
    vita_tone_abort = 1;
    return JAVACALL_OK;
}

/**
 * Get supported meta data key counts for specefic media type
 * 
 * @param mediaType [in]    Media type
 * @param keyCounts [out]   Return meta data key string counts
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_FAIL            Fail
 * @retval JAVACALL_NOT_IMPLEMENTED MetaDataControl is not implemented for this media type
 */
javacall_result javacall_media_get_metadata_key_counts(javacall_handle handle, /*OUT*/ long* keyCounts) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get meta data key strings by using media type and index value
 * 
 * @param mediaType [in]    Media type
 * @param index     [in]    Meta data key string's index value. from 0 to 'key counts - 1'.
 * @param bufLength [in]    keyBuf buffer's size in bytes. 
 * @param keyBuf    [out]   Buffer that used to return key strings. 
 *                          NULL value should be appended to the end of string.
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_OUT_OF_MEMORY     keyBuf size is too small
 * @retval JAVACALL_NOT_IMPLEMENTED  MetaDataControl is not implemented for this media type
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_metadata_key(javacall_handle handle, 
                                                long index, long bufLength, 
                                                /*OUT*/ char* keyBuf) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get meta data value strings by using meta data key string
 * 
 * @param hLIB      [in]    Handle to the library 
 * @param key       [in]    Meta data key string
 * @param bufLength [in]    dataBuf buffer's size in bytes. 
 * @param dataBuf   [out]   Buffer that used to return meta data strings. 
 *                          NULL value should be appended to the end of string.
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_OUT_OF_MEMORY     dataBuf size is too small
 * @retval JAVACALL_NOT_IMPLEMENTED  MetaDataControl is not implemented for this media type
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_metadata(javacall_handle handle, 
                                            const char* key, long bufLength, 
                                            /*OUT*/ char* dataBuf) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get volume for the given channel. 
 * The return value is independent of the master volume, which is set and retrieved with VolumeControl.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param volume    channel volume, 0-127, or -1 if not known
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_channel_volume(javacall_handle handle,
                                                  long channel, /*OUT*/ long* volume) {
    vita_player_t *p = (vita_player_t *)handle;
    (void)channel; /* un solo bus de mezcla en este backend */
    if (!p || !volume) {
        return JAVACALL_FAIL;
    }
    *volume = p->muted ? 0 : (p->volume * 127) / 100;
    return JAVACALL_OK;
}

/**
 * Set volume for the given channel. To mute, set to 0. 
 * This sets the current volume for the channel and may be overwritten during playback by events in a MIDI sequence.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param volume    channel volume, 0-127
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_channel_volume(javacall_handle handle,
                                                  long channel, long volume) {
    vita_player_t *p = (vita_player_t *)handle;
    (void)channel;
    if (!p) {
        return JAVACALL_FAIL;
    }
    /* Escala MIDI 0..127 -> 0..100. */
    return javacall_media_set_volume(handle, (volume * 100) / 127) >= 0
        ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Set program of a channel. 
 * This sets the current program for the channel and may be overwritten during playback by events in a MIDI sequence.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param bank      0-16383, or -1 for default bank
 * @param program   0-127
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_program(javacall_handle handle, 
                                           long channel, long bank, long program) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Sends a short MIDI event to the device.
 * 
 * @param handle    Handle to the library 
 * @param type      0x80..0xFF, excluding 0xF0 and 0xF7, which are reserved for system exclusive
 * @param data1     for 2 and 3-byte events: first data byte, 0..127
 * @param data2     for 3-byte events: second data byte, 0..127
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_short_midi_event(javacall_handle handle,
                                                long type, long data1, long data2) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Sends a long MIDI event to the device, typically a system exclusive message.
 * 
 * @param handle    Handle to the library 
 * @param data      array of the bytes to send. 
 *                  This memory buffer will be freed after this function returned.
 *                  So, you should copy this data to the other internal memory buffer
 *                  if this function needs data after return.
 * @param offset    start offset in data array
 * @param length    number of bytes to be sent
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_long_midi_event(javacall_handle handle,
                                               const char* data, long offset, /*INOUT*/ long* length) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get maximum rate of media type
 * 
 * @param mediaType [in]    Media type
 * @param maxRate   [out]   Maximum rate value for this media type
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_NOT_IMPLEMENTED  RateControl is not implemented for this media type
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_max_rate(javacall_handle handle, /*OUT*/ long* maxRate) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get minimum rate of media type
 * 
 * @param mediaType [in]    Media type
 * @param minRate   [out]   Minimum rate value for this media type
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_min_rate(javacall_handle handle, /*OUT*/ long* minRate) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Set media's current playing rate
 * 
 * @param hLIB [in] Handle to the library 
 * @param rate [in] Rate to set
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_rate(javacall_handle handle, long rate) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get media's current playing rate
 * 
 * @param hLIB [in]     Handle to the library 
 * @param rate [out]    Current playing rate
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_rate(javacall_handle handle, /*OUT*/ long* rate) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Get media's current playing tempo.
 * 
 * @param hLIB [in]     Handle to the library
 * @param tempo [out]   Current playing tempo
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_tempo(javacall_handle handle, /*OUT*/ long* tempo) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Set media's current playing tempo
 * 
 * @param hLIB [in]     Handle to the library
 * @param tempo [in]    Tempo to set
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_set_tempo(javacall_handle handle, long tempo) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/******************************************************************************/

/**
 * Gets the maximum playback pitch raise supported by the Player
 * 
 * @param handle    Handle to the library 
 * @param maxPitch  The maximum pitch raise in "milli-semitones".
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_max_pitch(javacall_handle handle, /*OUT*/ long* maxPitch) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets the minimum playback pitch raise supported by the Player
 * 
 * @param handle    Handle to the library 
 * @param minPitch  The minimum pitch raise in "milli-semitones"
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_min_pitch(javacall_handle handle, /*OUT*/ long* minPitch) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Set media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param pitch     The number of semi tones to raise the playback pitch. It is specified in "milli-semitones"
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_pitch(javacall_handle handle, long pitch) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param pitch     The current playback pitch raise in "milli-semitones"
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_pitch(javacall_handle handle, /*OUT*/ long* pitch) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/******************************************************************************/

/**
 * Query if recording is supported based on the player's content-type
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_supports_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Is this recording transaction is handled by native layer or Java layer?
 * 
 * @param handle    Handle to the library 
 * @param locator   URL locator string for recording data (ex: file:///root/test.wav)
 * 
 * @retval JAVACALL_OK      This recording transaction will be handled by native layer
 * @retval JAVACALL_FAIL    This recording transaction should be handled by Java layer
 */
javacall_result javacall_media_recording_handled_by_native(javacall_handle handle, 
                                                           const javacall_utf16* locator,
                                                           long locatorLength) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Is javacall_media_set_recordsize_limit function is working for this player?
 * 
 * @retval JAVACALL_TRUE    Yes. Supported.
 * @retval JAVACALL_FALSE   No. Not supported.
 */
javacall_bool javacall_media_set_recordsize_limit_supported(javacall_handle handle) {
    return JAVACALL_FALSE;
}

/**
 * Specify the maximum size of the recording including any headers.
 * If a size of -1 is passed then the record size limit should be removed.
 * 
 * @param handle    Handle to the library 
 * @param size      The maximum size bytes of the recording requested as input parameter.
 *                  The supported maximum size bytes of the recording which is less than or 
 *                  equal to the requested size as output parameter.
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_set_recordsize_limit(javacall_handle handle, 
                                                    /*INOUT*/ long* size) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Starts the recording. records all the data of the player ( video / audio )
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_start_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Pause the recording. this should enable a future call to javacall_media_start_recording. 
 * Another call to javacall_media_start_recording after pause has been called will result 
 * in recording the new data and concatanating it to the previously recorded data.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_pause_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Stop the recording.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_stop_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * The recording that has been done so far should be discarded. (deleted)
 * Recording will be paused before this method is called. 
 * If javacall_media_start_recording is called after this method is called, recording should resume.
 * Calling reset after javacall_media_finish_recording will have no effect on the current recording.
 * If the Player that is associated with this RecordControl is closed, 
 * javacall_media_reset_recording will be called implicitly. 
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_reset_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * The recording should be completed; 
 * this may involve updating the header,flushing buffers and closing the temporary file if it is used
 * by the implementation.
 * javacall_media_pause_recording will be called before this method is called.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_commit_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get how much data was returned. 
 * This function can be called after a successful call to javacall_media_finish_recording.
 * 
 * @param handle    Handle to the library 
 * @param size      How much data was recorded
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_recorded_data_size(javacall_handle handle, 
                                                      /*OUT*/ long* size) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets the recorded data.
 * This function can be called after a successful call to javacall_media_finish_recording.
 * It receives the data recorded from offset till the size.
 * 
 * @param handle    Handle to the library 
 * @param buffer    Buffer will contains the recorded data
 * @param offset    An offset to the start of the required recorded data
 * @param size      How much data will be copied to buffer
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_recorded_data(javacall_handle handle, 
                                                 /*OUT*/ char* buffer, long offset, long size) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get the current recording data content type mime string length
 *
 * @return  If success return length of string else return 0
 */
int javacall_media_get_record_content_type_length(javacall_handle handle) {
    return 0;
}

/**
 * Get the current recording data content type mime string length
 * For example : 'audio/x-wav' for audio recording
 *
 * @param handle                Handle of native player
 * @param contentTypeBuf        Buffer to return content type unicode string
 * @param contentTypeBufLength  Lenght of contentTypeBuf buffer (in unicode metrics)
 *
 * @return  Length of content type string stored in contentTypeBuf
 */
int javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           int contentTypeBufLength) {
    return 0;
}

/**
 * Close the recording. Delete all resources related with this recording.
 * 
 * @param handle    Handle to the library 
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_close_recording(javacall_handle handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * This function called by JVM when this player goes to foreground.
 * There is only one foreground midlets but, multiple player can be exits at this midlets.
 * So, there could be multiple players from JVM.
 * Device resource handling policy is not part of Java implementation. It is totally depends on
 * native layer's implementation.
 * 
 * @param handle    Handle to the native player
 * @param option    MVM options. Check about javacall_media_mvm_option type definition.
 * 
 * @retval JAVACALL_OK  Somthing happened
 * @retval JAVACALL_OK  Nothing happened
 */
javacall_result javacall_media_to_foreground(javacall_handle handle,
                                             javacall_media_mvm_option option) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * This function called by JVM when this player goes to background.
 * There could be multiple background midlets. Also, multiple player can be exits at this midlets.
 * Device resource handling policy is not part of Java implementation. It is totally depends on
 * native layer's implementation.
 * 
 * @param handle    Handle to the native player
 * @param option    MVM options. Check about javacall_media_mvm_option type definition.
 * 
 * @retval JAVACALL_OK  Somthing happened
 * @retval JAVACALL_OK  Nothing happened
 */
javacall_result javacall_media_to_background(javacall_handle handle,
                                             javacall_media_mvm_option option) {
    return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * This function is used to ascertain the availability of MIDI bank support
 *
 * @param handle     Handle to the native player
 * @param supported  return of support availability
 *
 * @retval JAVACALL_OK      MIDI Bank Query support is available
 * @retval JAVACALL_FAIL    NO MIDI Bank Query support is available
 */
javacall_result javacall_media_is_midibank_query_supported(javacall_handle handle,
                                             long* supported) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * This function is used to get a list of installed banks. If the custom
 * parameter is true, a list of custom banks is returned. Otherwise, a list of
 * all banks (custom and internal) is returned. This function can be left empty.
 *
 * @param handle    Handle to the native player
 * @param custom    a flag indicating whether to return just custom banks, or
 *                  all banks.
 * @param banklist  an array which will be filled out with the banklist
 * @param numlist   the length of the array to be filled out, and on return
 *                  contains the number of values written to the array.
 *
 * @retval JAVACALL_OK      Bank List is available
 * @retval JAVACALL_FAIL    Bank List is NOT available
 */
javacall_result javacall_media_get_midibank_list(javacall_handle handle,
                                             long custom,
                                             /*OUT*/short* banklist,
                                             /*INOUT*/long* numlist) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Given a bank, program and key, get name of key. This function applies to
 * key-mapped banks (i.e. percussive banks or effect banks) only. If the returned
 * keyname length is 0, then the key is not mapped to a sound. For melodic banks,
 * where each key (=note) produces the same sound at different pitch, this
 * function always returns a 0 length string. For space saving reasons an
 * implementation may return a 0 length string instead of the keyname. This
 * can be left empty.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank to query
 * @param program   The program to query
 * @param key       The key to query
 * @param keyname   The name of the key returned.
 * @param keynameLen    The length of the keyname array, and on return the
 *                      length of the keyname.
 *
 * @retval JAVACALL_OK      Keyname available
 * @retval JAVACALL_FAIL    Keyname not supported
 */
javacall_result javacall_media_get_midibank_key_name(javacall_handle handle,
                                            long bank,
                                            long program,
                                            long key,
                                            /*OUT*/char* keyname,
                                            /*INOUT*/long* keynameLen) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Given the bank and program, get name of program. For space-saving reasons
 * a 0 length string may be returned.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank being queried
 * @param program   The program being queried
 * @param progname  The name of the program returned
 * @param prognameLen    The length of the progname array, and on return the
 *                       length of the progname
 *
 * @retval JAVACALL_OK      Program name available
 * @retval JAVACALL_FAIL    Program name not supported
 */
javacall_result javacall_media_get_midibank_program_name(javacall_handle handle,
                                                long bank,
                                                long program,
                                                /*OUT*/char* progname,
                                                /*INOUT*/long* prognameLen) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Given bank, get list of program numbers. If and only if this bank is not
 * installed, an empty array is returned.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank being queried
 * @param proglist  The Program List being returned
 * @param proglistLen     The length of the proglist, and on return the number
 *                        of program numbers in the list
 *
 * @retval JAVACALL_OK     Program list available
 * @retval JAVACALL_FAIL   Program list unsupported
 */
javacall_result javacall_media_get_midibank_program_list(javacall_handle handle,
                                                long bank,
                                                /*OUT*/char* proglist,
                                                /*INOUT*/long* proglistLen) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Returns the program assigned to the channel. It represents the current state
 * of the channel. During playbank of the MIDI file, the program may change due
 * to program change events in the MIDI file. The returned array is represented
 * by an array {bank, program}. The support of this function is optional.
 *
 * @param handle    Handle to the native player
 * @param channel   The channel being queried
 * @param prog      The return array (size 2) in the form {bank, program}
 *
 * @retval JAVACALL_OK    Program available
 * @retval JAVACALL_FAIL  Get Program unsupported
 */
javacall_result javacall_media_get_midibank_program(javacall_handle handle,
                                                long channel,
                                                /*OUT*/long* prog) {
    return JAVACALL_NOT_IMPLEMENTED;
}

#ifdef __cplusplus
}
#endif 
 
