/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

#include "SDL_vitaaudio.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/audioout.h>
#include <malloc.h>

/* The tag name used by DUMMY audio */
#define VITAAUD_DRIVER_NAME         "vita"

#define SDL_AUDIO_MASK_BITSIZE		(0xFF)
#define SDL_AUDIO_BITSIZE(x)		(x & SDL_AUDIO_MASK_BITSIZE)

#define SCE_AUDIO_SAMPLE_ALIGN(s)	(((s) + 63) & ~63)
#define SCE_AUDIO_MAX_VOLUME		0x8000

/* Audio driver functions */
static int VITAAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void VITAAUD_WaitAudio(_THIS);
static void VITAAUD_PlayAudio(_THIS);
static Uint8 *VITAAUD_GetAudioBuf(_THIS);
static void VITAAUD_CloseAudio(_THIS);

/* Audio driver bootstrap functions */
static int VITAAUD_Available(void)
{
    return(1);
}

static void VITAAUD_ThreadInit(_THIS)
{
    /* Increase the priority of this audio thread by 1 to put it
       ahead of other SDL threads. */
    SceUID thid;
    SceKernelThreadInfo info;
    thid = sceKernelGetThreadId();
    info.size = sizeof(SceKernelThreadInfo);
    if (sceKernelGetThreadInfo(thid, &info) == 0) {
        sceKernelChangeThreadPriority(thid, info.currentPriority - 1);
    }
}

static void VITAAUD_DeleteDevice(SDL_AudioDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_AudioDevice *VITAAUD_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;

    /* Initialize all variables that we clean on shutdown */
    this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
    if ( this ) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)SDL_malloc((sizeof *this->hidden));
    }
    if ( (this == NULL) || (this->hidden == NULL) ) {
        SDL_OutOfMemory();
        if ( this ) {
            SDL_free(this);
        }
        return(0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Set the function pointers */
    this->OpenAudio = VITAAUD_OpenAudio;
    this->WaitAudio = VITAAUD_WaitAudio;
    this->PlayAudio = VITAAUD_PlayAudio;
    this->GetAudioBuf = VITAAUD_GetAudioBuf;
    this->CloseAudio = VITAAUD_CloseAudio;
    this->ThreadInit = VITAAUD_ThreadInit;

    this->free = VITAAUD_DeleteDevice;

    return this;
}

AudioBootStrap VITAAUD_bootstrap = {
    VITAAUD_DRIVER_NAME, "SDL vita audio driver",
    VITAAUD_Available, VITAAUD_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void VITAAUD_WaitAudio(_THIS)
{
}

static void VITAAUD_PlayAudio(_THIS)
{
    Uint8 *mixbuf = this->hidden->mixbufs[this->hidden->next_buffer];

    int vols[2] = {SCE_AUDIO_MAX_VOLUME, SCE_AUDIO_MAX_VOLUME};
    sceAudioOutSetVolume(this->hidden->channel, SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
    sceAudioOutOutput(this->hidden->channel, mixbuf);

    this->hidden->next_buffer = (this->hidden->next_buffer + 1) % NUM_BUFFERS;
}

static Uint8 *VITAAUD_GetAudioBuf(_THIS)
{
        return this->hidden->mixbufs[this->hidden->next_buffer];
}

static void VITAAUD_CloseAudio(_THIS)
{
    if (this->hidden->channel >= 0) {
        sceAudioOutReleasePort(this->hidden->channel);
        this->hidden->channel = -1;
    }

    if (this->hidden->rawbuf != NULL) {
        free(this->hidden->rawbuf);
        this->hidden->rawbuf = NULL;
    }
}

static int VITAAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    int format, mixlen, i, port = SCE_AUDIO_OUT_PORT_TYPE_MAIN;

    switch (spec->format & 0xff) {
        case 8:
        case 16:
            spec->format = AUDIO_S16LSB;
            break;
        default:
            SDL_SetError("Unsupported audio format");
            return -1;
    }

    /* The sample count must be a multiple of 64. */
    spec->samples = SCE_AUDIO_SAMPLE_ALIGN(spec->samples);

    SDL_CalculateAudioSpec(spec);

    /* Allocate the mixing buffer.  Its size and starting address must
       be a multiple of 64 bytes.  Our sample count is already a multiple of
       64, so spec->size should be a multiple of 64 as well. */

    mixlen = spec->size * NUM_BUFFERS;
    this->hidden->rawbuf = (Uint8 *) memalign(64, mixlen);
    if (this->hidden->rawbuf == NULL) {
        SDL_SetError("Couldn't allocate mixing buffer");
        return -1;
    }

    /* Setup the hardware channel. */
    if (spec->channels == 1) {
        format = SCE_AUDIO_OUT_MODE_MONO;
    } else {
        format = SCE_AUDIO_OUT_MODE_STEREO;
    }


    if(spec->freq < 48000) {
        port = SCE_AUDIO_OUT_PORT_TYPE_BGM;
    }

    this->hidden->channel = sceAudioOutOpenPort(port, spec->samples, spec->freq, format);
    if (this->hidden->channel < 0) {
        free(this->hidden->rawbuf);
        this->hidden->rawbuf = NULL;
        SDL_SetError("Couldn't reserve hardware channel");
        return -1;
    }

    SDL_memset(this->hidden->rawbuf, 0, mixlen);
    for (i = 0; i < NUM_BUFFERS; i++) {
        this->hidden->mixbufs[i] = &this->hidden->rawbuf[i * spec->size];
    }

    this->hidden->next_buffer = 0;
    return 0;
}

