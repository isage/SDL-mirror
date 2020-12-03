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

#include "SDL_keyboard.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_sysevents.h"

#include "SDL_vitavideo.h"
#include "SDL_vitakeyboard_c.h"

SceHidKeyboardReport k_reports[SCE_HID_MAX_REPORT];
int keyboard_hid_handle = 0;
Uint8 prev_keys[6] = {0};
Uint8 prev_modifiers = 0;
Uint8 locks = 0;

const Uint8 numkeys = 0xE8;
const int keymap[0xE8] = {
    0, 0, 0, 0, SDLK_a, SDLK_b, SDLK_c, SDLK_d,
    SDLK_e, SDLK_f, SDLK_g, SDLK_h, SDLK_i, SDLK_j, SDLK_k, SDLK_l,
    SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q, SDLK_r, SDLK_s, SDLK_t,
    SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z, SDLK_1, SDLK_2,
    SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0,
    SDLK_RETURN, SDLK_ESCAPE, SDLK_BACKSPACE, SDLK_TAB, SDLK_SPACE, SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET,
    SDLK_RIGHTBRACKET, SDLK_BACKSLASH, SDLK_HASH, SDLK_SEMICOLON, SDLK_BACKQUOTE, SDLK_QUOTE, SDLK_COMMA, SDLK_PERIOD,
    SDLK_SLASH, SDLK_CAPSLOCK, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
    SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_PRINT, SDLK_SCROLLOCK,
    SDLK_BREAK, SDLK_INSERT, SDLK_HOME, SDLK_PAGEUP, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN, SDLK_RIGHT,
    SDLK_LEFT, SDLK_DOWN, SDLK_UP, SDLK_NUMLOCK, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP_PLUS,
    SDLK_KP_ENTER, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
    SDLK_KP8, SDLK_KP9, SDLK_KP0, SDLK_KP_PERIOD, SDLK_KP_EQUALS, SDLK_F13, SDLK_F14, SDLK_F15,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    SDLK_LCTRL, SDLK_LSHIFT, SDLK_LALT, SDLK_LMETA, SDLK_RCTRL, SDLK_RSHIFT, SDLK_RALT, SDLK_RMETA
};

const int SCANCODE_NUMLOCKCLEAR = 0x53;
const int SCANCODE_CAPSLOCK = 0x39;
const int SCANCODE_SCROLLLOCK = 0x47;
const int SCANCODE_LCTRL = 0xE0;
const int SCANCODE_LSHIFT = 0xE1;
const int SCANCODE_LALT = 0xE2;
const int SCANCODE_LGUI = 0xE3;
const int SCANCODE_RCTRL = 0xE4;
const int SCANCODE_RSHIFT = 0xE5;
const int SCANCODE_RALT = 0xE6;
const int SCANCODE_RGUI = 0xE7;

SDL_keysym *VITA_TranslateKey(int scancode, SDL_keysym *keysym, SDL_bool pressed)
{
    int asciicode = 0;

    int translated_keysym = SDLK_UNKNOWN;

    if (scancode < numkeys)
        translated_keysym = keymap[scancode];
        
    /* Set the keysym information */
    keysym->scancode = scancode;
    keysym->mod = KMOD_NONE;
    keysym->sym = translated_keysym;
    keysym->unicode = 0;

    if (SDL_TranslateUNICODE && pressed && translated_keysym <= 127) {
        keysym->unicode = translated_keysym;
    }

    return(keysym);
}

void 
VITA_InitKeyboard(void)
{
    sceHidKeyboardEnumerate(&keyboard_hid_handle, 1);
}

void 
VITA_PollKeyboard(void)
{
    SDL_keysym keysym;
    if (keyboard_hid_handle > 0)
    {
        // Capslock and Numlock keys only change state on SDL_PRESSED

        int numReports = sceHidKeyboardRead(keyboard_hid_handle, (SceHidKeyboardReport**)&k_reports, SCE_HID_MAX_REPORT);

        if (numReports < 0) {
            keyboard_hid_handle = 0;
        }
        else if (numReports) {

            if (k_reports[numReports - 1].modifiers[1] & 0x1) {
                if (!(locks & 0x1)) {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_NUMLOCKCLEAR, &keysym, 1));
                    locks |= 0x1;
                }
            }
            else {
                if (locks & 0x1) {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_NUMLOCKCLEAR, &keysym, 0));
                    locks &= ~0x1;
                }
            }

            if (k_reports[numReports - 1].modifiers[1] & 0x2) {
                if (!(locks & 0x2)) {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_CAPSLOCK, &keysym, 1));
                    locks |= 0x2;
                }
            }
            else {
                if (locks & 0x2) {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_CAPSLOCK, &keysym, 0));
                    locks &= ~0x2;
                }
            }

            if (k_reports[numReports - 1].modifiers[1] & 0x4) {
                if (!(locks & 0x4)) {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_SCROLLLOCK, &keysym, 1));
                    locks |= 0x4;
                }
            }
            else {
                if (locks & 0x4) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_SCROLLLOCK, &keysym, 0));
                    locks &= ~0x4;
                }
            }

            Uint8 changed_modifiers = k_reports[numReports - 1].modifiers[0] ^ prev_modifiers;

            if (changed_modifiers & 0x01) {
                if (prev_modifiers & 0x01) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_LCTRL, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_LCTRL, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x02) {
                if (prev_modifiers & 0x02) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_LSHIFT, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_LSHIFT, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x04) {
                if (prev_modifiers & 0x04) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_LALT, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_LALT, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x08) {
                if (prev_modifiers & 0x08) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_LGUI, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_LGUI, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x10) {
                if (prev_modifiers & 0x10) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_RCTRL, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_RCTRL, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x20) {
                if (prev_modifiers & 0x20) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_RSHIFT, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_RSHIFT, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x40) {
                if (prev_modifiers & 0x40) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_RALT, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_RALT, &keysym, 1));
                }
            }
            if (changed_modifiers & 0x80) {
                if (prev_modifiers & 0x80) {
                    SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(SCANCODE_RGUI, &keysym, 0));
                }
                else {
                    SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(SCANCODE_RGUI, &keysym, 1));
                }
            }

            prev_modifiers = k_reports[numReports - 1].modifiers[0];

            for (int i = 0; i < 6; i++) {

                int keyCode = k_reports[numReports - 1].keycodes[i];

                if (keyCode != prev_keys[i]) {
                    if (prev_keys[i]) {
                        SDL_PrivateKeyboard(SDL_RELEASED, VITA_TranslateKey(prev_keys[i], &keysym, 0));
                    }
                    if (keyCode) {
                        SDL_PrivateKeyboard(SDL_PRESSED, VITA_TranslateKey(keyCode, &keysym, 1));
                    }
                    prev_keys[i] = keyCode;
                }
            }
        }
    }
}
