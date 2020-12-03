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

#if SDL_JOYSTICK_VITA

/* This is the system specific header for the SDL joystick API */

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <psp2/types.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>

/* Current pad state */
static SceCtrlData pad0 = { .lx = 0, .ly = 0, .rx = 0, .ry = 0, .buttons = 0 };
static SceCtrlData pad1 = { .lx = 0, .ly = 0, .rx = 0, .ry = 0, .buttons = 0 };
static SceCtrlData pad2 = { .lx = 0, .ly = 0, .rx = 0, .ry = 0, .buttons = 0 };
static SceCtrlData pad3 = { .lx = 0, .ly = 0, .rx = 0, .ry = 0, .buttons = 0 };

static int port_map[4]= { 0, 2, 3, 4 }; //index: SDL joy number, entry: Vita port number

static const unsigned int button_map[] = {
    SCE_CTRL_TRIANGLE, SCE_CTRL_CIRCLE, SCE_CTRL_CROSS, SCE_CTRL_SQUARE,
    SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
    SCE_CTRL_DOWN, SCE_CTRL_LEFT, SCE_CTRL_UP, SCE_CTRL_RIGHT,
    SCE_CTRL_SELECT, SCE_CTRL_START};

static int analog_map[256];  /* Map analog inputs to -32768 -> 32767 */

typedef struct
{
  int x;
  int y;
} point;

/* 4 points define the bezier-curve. */
/* The Vita has a good amount of analog travel, so use a linear curve */
static point a = { 0, 0 };
static point b = { 0, 0  };
static point c = { 128, 32767 };
static point d = { 128, 32767 };

/* simple linear interpolation between two points */
static inline void lerp (point *dest, point *a, point *b, float t)
{
    dest->x = a->x + (b->x - a->x)*t;
    dest->y = a->y + (b->y - a->y)*t;
}

/* evaluate a point on a bezier-curve. t goes from 0 to 1.0 */
static int calc_bezier_y(float t)
{
    point ab, bc, cd, abbc, bccd, dest;
    lerp (&ab, &a, &b, t);           /* point between a and b */
    lerp (&bc, &b, &c, t);           /* point between b and c */
    lerp (&cd, &c, &d, t);           /* point between c and d */
    lerp (&abbc, &ab, &bc, t);       /* point between ab and bc */
    lerp (&bccd, &bc, &cd, t);       /* point between bc and cd */
    lerp (&dest, &abbc, &bccd, t);   /* point on the bezier-curve */
    return dest.y;
}

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
    int i;

    /* Setup input */
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

    /* Create an accurate map from analog inputs (0 to 255)
       to SDL joystick positions (-32768 to 32767) */
    for (i = 0; i < 128; i++)
    {
        float t = (float)i/127.0f;
        analog_map[i+128] = calc_bezier_y(t);
        analog_map[127-i] = -1 * analog_map[i+128];
    }

    SceCtrlPortInfo myPortInfo;

    // Assume we have at least one controller, even when nothing is paired
    // This way the user can jump in, pair a controller
    // and control things immediately even if it is paired
    // after the app has already started.

    SDL_numjoysticks = 1; 

    //How many additional paired controllers are there?
    sceCtrlGetControllerPortInfo(&myPortInfo);
    //On Vita TV, port 0 and 1 are the same controller
    //and that is the first one, so start at port 2
    for (i=2; i<=4; i++)
    {
        if (myPortInfo.port[i]!=SCE_CTRL_TYPE_UNPAIRED)
        {
            SDL_numjoysticks++;
        }
    }
   return SDL_numjoysticks;
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
    if (index >= 0 && index < 4)
        return "PSVita Controller";

    SDL_SetError("No joystick available with that index");
    return(NULL);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
    joystick->nbuttons = sizeof(button_map)/sizeof(*button_map);
    joystick->naxes = 4;
    joystick->nhats = 0;

    return 0;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
    int i;
    unsigned int buttons;
    unsigned int changed;
    unsigned char lx, ly, rx, ry;
    static unsigned int old_buttons[] = { 0, 0, 0, 0 };
    static unsigned char old_lx[] = { 0, 0, 0, 0 };
    static unsigned char old_ly[] = { 0, 0, 0, 0 };
    static unsigned char old_rx[] = { 0, 0, 0, 0 };
    static unsigned char old_ry[] = { 0, 0, 0, 0 };
    SceCtrlData *pad = NULL;

    int index = joystick->index;

    switch (index) {
        case 0:
            pad = &pad0;
            break;
        case 1:
            pad = &pad1;
            break;
        case 2:
            pad = &pad2;
            break;
        case 3:
            pad = &pad3;
            break;
        default:
            return;
    }

    sceCtrlPeekBufferPositive(port_map[index], pad, 1); 

    buttons = pad->buttons;
    lx = pad->lx;
    ly = pad->ly;
    rx = pad->rx;
    ry = pad->ry;

    /* Axes */
    if(old_lx[index] != lx) {
        SDL_PrivateJoystickAxis(joystick, 0, analog_map[lx]);
        old_lx[index] = lx;
    }
    if(old_ly[index] != ly) {
        SDL_PrivateJoystickAxis(joystick, 1, analog_map[ly]);
        old_ly[index] = ly;
    }
    if(old_rx[index] != rx) {
        SDL_PrivateJoystickAxis(joystick, 2, analog_map[rx]);
        old_rx[index] = rx;
    }
    if(old_ry[index] != ry) {
        SDL_PrivateJoystickAxis(joystick, 3, analog_map[ry]);
        old_ry[index] = ry;
    }

    /* Buttons */
    changed = old_buttons[index] ^ buttons;
    old_buttons[index] = buttons;
    if(changed) {
        for(i=0; i<sizeof(button_map)/sizeof(button_map[0]); i++) {
            if(changed & button_map[i]) {
                SDL_PrivateJoystickButton(
                    joystick, i,
                    (buttons & button_map[i]) ? 1 : 0);
            }
        }
    }
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
    return;
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
    return;
}

#endif /* SDL_JOYSTICK_VITA */
