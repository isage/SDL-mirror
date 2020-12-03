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
*/
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_vitavideo.h"
#include "SDL_vitaevents_c.h"
#include "SDL_vitamouse_c.h"
#include "SDL_vitakeyboard_c.h"

#define VITAVID_DRIVER_NAME "vita"

#define SCREEN_W 960
#define SCREEN_H 544
#define ALIGN(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define DISPLAY_PIXEL_FORMAT SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

static int vsync = 0;

void *vita_gpu_alloc(SceKernelMemBlockType type, unsigned int size, SceUID *uid);
void vita_gpu_free(SceUID uid);

/* Initialization/Query functions */
static int VITA_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **VITA_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *VITA_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int VITA_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void VITA_VideoQuit(_THIS);

/* Hardware surface functions */
static int VITA_FlipHWSurface(_THIS, SDL_Surface *surface);
static int VITA_AllocHWSurface(_THIS, SDL_Surface *surface);
static int VITA_LockHWSurface(_THIS, SDL_Surface *surface);
static void VITA_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void VITA_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void VITA_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

void *vita_gpu_alloc(SceKernelMemBlockType type, unsigned int size, SceUID *uid)
{
    void *mem;

    if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
        size = ALIGN(size, 256*1024);
    } else {
        size = ALIGN(size, 4*1024);
    }

    *uid = sceKernelAllocMemBlock("gpu_mem", type, size, NULL);

    if (*uid < 0)
        return NULL;

    if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
        return NULL;

    return mem;
}

void gpu_free(SceUID uid)
{
    void *mem = NULL;
    if (sceKernelGetMemBlockBase(uid, &mem) < 0)
        return;
    sceKernelFreeMemBlock(uid);
}

/* VITA driver bootstrap functions */
static int VITA_Available(void)
{
    return 1;
}

static void VITA_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_VideoDevice *VITA_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
    if ( device ) {
        SDL_memset(device, 0, (sizeof *device));
        device->hidden = (struct SDL_PrivateVideoData *)
                SDL_malloc((sizeof *device->hidden));
        SDL_memset(device->hidden, 0, (sizeof *device->hidden));
    }
    if ( (device == NULL) || (device->hidden == NULL) ) {
        SDL_OutOfMemory();
        if ( device ) {
            SDL_free(device);
        }
        return(0);
    }
    SDL_memset(device->hidden, 0, (sizeof *device->hidden));

    /* Set the function pointers */
    device->VideoInit = VITA_VideoInit;
    device->ListModes = VITA_ListModes;
    device->SetVideoMode = VITA_SetVideoMode;
    device->CreateYUVOverlay = NULL;
    device->SetColors = VITA_SetColors;
    device->UpdateRects = VITA_UpdateRects;
    device->VideoQuit = VITA_VideoQuit;
    device->AllocHWSurface = VITA_AllocHWSurface;
    device->CheckHWBlit = NULL;
    device->FillHWRect = NULL;
    device->SetHWColorKey = NULL;
    device->SetHWAlpha = NULL;
    device->LockHWSurface = VITA_LockHWSurface;
    device->UnlockHWSurface = VITA_UnlockHWSurface;
    device->FlipHWSurface = VITA_FlipHWSurface;
    device->FreeHWSurface = VITA_FreeHWSurface;
    device->SetCaption = NULL;
    device->SetIcon = NULL;
    device->IconifyWindow = NULL;
    device->GrabInput = NULL;
    device->GetWMInfo = NULL;
    device->InitOSKeymap = VITA_InitOSKeymap;
    device->PumpEvents = VITA_PumpEvents;

    device->free = VITA_DeleteDevice;

    return device;
}

int VITA_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    vformat->BitsPerPixel = 32;
    vformat->BytesPerPixel = 4;
    vformat->Rmask = 0x000000FF;
    vformat->Gmask = 0x0000FF00;
    vformat->Bmask = 0x00FF0000;
    vformat->Amask = 0xFF000000;

    VITA_InitKeyboard();
    VITA_InitMouse();

    return(0);
}

SDL_Surface *VITA_SetVideoMode(_THIS, SDL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    SceDisplayFrameBuf param;
    param.size = sizeof(SceDisplayFrameBuf);
    sceDisplayGetFrameBuf(&param, SCE_DISPLAY_SETBUF_IMMEDIATE);

    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        this->hidden->buffer[i] = vita_gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
            4 * SCREEN_W * SCREEN_H,
            &this->hidden->buffer_uid[i]
        );

        // memset the buffer to black
        for (int y = 0; y < SCREEN_H; y++) {
            unsigned int *row = (unsigned int *)this->hidden->buffer[i] + y*SCREEN_W;
            for (int x = 0; x < SCREEN_W; x++) {
                row[x] = 0xff0000FF;
            }
        }
    }
    this->hidden->buffer_index = 0;

    SceDisplayFrameBuf framebuf;
    SDL_memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
    framebuf.size        = sizeof(SceDisplayFrameBuf);
    framebuf.base        = this->hidden->buffer[this->hidden->buffer_index];
    framebuf.pitch       = SCREEN_W;
    framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
    framebuf.width       = SCREEN_W;
    framebuf.height      = SCREEN_H;
    int err = sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

    current->flags = (SDL_FULLSCREEN | SDL_HWSURFACE);

    current->w = SCREEN_W;
    current->h = SCREEN_H;
    current->pitch = SCREEN_W * 4;

    if ( ! SDL_ReallocFormat(current, 32, 0x000000FF, 0x0000ff00, 0x00ff0000, 0xff000000) ) {
        return(NULL);
    }
    current->pixels = this->hidden->buffer[0];

    if ((flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
        current->flags |= SDL_DOUBLEBUF;
        // draw on backbuffer
        int buffer_index = (this->hidden->buffer_index + 1) % DISPLAY_BUFFER_COUNT;
        current->pixels = this->hidden->buffer[buffer_index];
    }

    return(current);
}

SDL_Rect **VITA_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    static SDL_Rect VITA_Rects[] = {
        {0, 0, 960, 544},
    };
    static SDL_Rect *VITA_modes[] = {
        &VITA_Rects[0],
        NULL
    };
    SDL_Rect **modes = VITA_modes;

    switch(format->BitsPerPixel)
    {
        case 16:
        case 24:
        case 32:
        return modes;

        default:
        return (SDL_Rect **) -1;
    }
}

static int VITA_AllocHWSurface(_THIS, SDL_Surface *surface)
{
    return(-1);
}

static void VITA_FreeHWSurface(_THIS, SDL_Surface *surface)
{
}

static int VITA_FlipHWSurface(_THIS, SDL_Surface *surface)
{
    if ((surface->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
        SceDisplayFrameBuf framebuf;
        SDL_memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
        framebuf.size        = sizeof(SceDisplayFrameBuf);
        framebuf.base        = this->hidden->buffer[this->hidden->buffer_index];
        framebuf.pitch       = SCREEN_W;
        framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
        framebuf.width       = SCREEN_W;
        framebuf.height      = SCREEN_H;
        int err = sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

        this->hidden->buffer_index = (this->hidden->buffer_index + 1) % DISPLAY_BUFFER_COUNT;
        surface->pixels = this->hidden->buffer[this->hidden->buffer_index];
    }
}

static int VITA_LockHWSurface(_THIS, SDL_Surface *surface)
{
    return(0);
}

static void VITA_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

static void VITA_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    /* do nothing? */
}

int VITA_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    return(1);
}

void VITA_VideoQuit(_THIS)
{
}

VideoBootStrap VITA_bootstrap = {
    VITAVID_DRIVER_NAME, "SDL vita video driver",
    VITA_Available, VITA_CreateDevice
};

