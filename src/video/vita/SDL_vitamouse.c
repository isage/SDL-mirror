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

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/hid.h>

#include "SDL_mouse.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_sysevents.h"

#include "SDL_vitavideo.h"
#include "SDL_vitamouse_c.h"

SceHidMouseReport m_reports[SCE_HID_MAX_REPORT];
int mouse_hid_handle = 0;
Uint8 prev_buttons = 0;

void 
VITA_InitMouse(void)
{
    sceHidMouseEnumerate(&mouse_hid_handle, 1);
}

void 
VITA_PollMouse(void)
{
    if (mouse_hid_handle > 0)
    {
        int numReports = sceHidMouseRead(mouse_hid_handle, (SceHidMouseReport**)&m_reports, SCE_HID_MAX_REPORT);
        if (numReports > 0)
        {
            for (int i = 0; i <= numReports - 1; i++)
            {
                Uint8 changed_buttons = m_reports[i].buttons ^ prev_buttons;

                if (changed_buttons & 0x1) {
                    if (prev_buttons & 0x1)
                        SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);
                    else
                        SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_LEFT, 0, 0);
                }
                if (changed_buttons & 0x2) {
                    if (prev_buttons & 0x2)
                        SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);
                    else
                        SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_RIGHT, 0, 0);
                }
                if (changed_buttons & 0x4) {
                    if (prev_buttons & 0x4)
                        SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_MIDDLE, 0, 0);
                    else
                        SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_MIDDLE, 0, 0);
                }

                prev_buttons = m_reports[i].buttons;

                if (m_reports[i].rel_x || m_reports[i].rel_y) 
                {
                    SDL_PrivateMouseMotion(0, 1, m_reports[i].rel_x, m_reports[i].rel_y);
                }
            }
        }
    }
}

