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

#ifndef _SDL_vitavideo_h
#define _SDL_vitavideo_h

#include "../SDL_sysvideo.h"
#include <psp2/types.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

/* Hidden "this" pointer for the video functions */
#define _THIS   SDL_VideoDevice *this

#define DISPLAY_BUFFER_COUNT 2

/* Private display data */

struct SDL_PrivateVideoData {
    uint8_t buffer_index;
    SceUID buffer_uid[DISPLAY_BUFFER_COUNT];
    void* buffer[DISPLAY_BUFFER_COUNT];
};

#endif /* _SDL_vitavideo_h */
