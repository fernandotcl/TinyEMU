/*
 * SDL display driver
 *
 * Copyright (c) 2017 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#include <SDL2/SDL.h>

#include "cutils.h"
#include "virtio.h"
#include "machine.h"

#define KEYCODE_MAX 127

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *fb_texture;
static int window_width, window_height, fb_width, fb_height;
static SDL_Cursor *sdl_cursor_hidden;
static uint8_t key_pressed[KEYCODE_MAX + 1];

static void sdl_update_fb_texture(FBDevice *fb_dev)
{
    if (!fb_texture ||
        fb_width != fb_dev->width ||
        fb_height != fb_dev->height) {

        if (fb_texture != NULL)
            SDL_DestroyTexture(fb_texture);

        fb_width = fb_dev->width;
        fb_height = fb_dev->height;

        fb_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       fb_dev->width,
                                       fb_dev->height);
        if (!fb_texture) {
            fprintf(stderr, "Could not create SDL framebuffer texture\n");
            exit(1);
        }
    }
}

static void sdl_update(FBDevice *fb_dev, void *opaque,
                       int x, int y, int w, int h)
{
    int *dirty = (int *)opaque;
    *dirty = 1;
}

static int sdl_get_keycode(const SDL_KeyboardEvent *ev)
{
    switch (ev->keysym.scancode) {
        case SDL_SCANCODE_ESCAPE: return 1;
        case SDL_SCANCODE_1: return 2;
        case SDL_SCANCODE_2: return 3;
        case SDL_SCANCODE_3: return 4;
        case SDL_SCANCODE_4: return 5;
        case SDL_SCANCODE_5: return 6;
        case SDL_SCANCODE_6: return 7;
        case SDL_SCANCODE_7: return 8;
        case SDL_SCANCODE_8: return 9;
        case SDL_SCANCODE_9: return 10;
        case SDL_SCANCODE_0: return 11;
        case SDL_SCANCODE_MINUS: return 12;
        case SDL_SCANCODE_EQUALS: return 13;
        case SDL_SCANCODE_BACKSPACE: return 14;
        case SDL_SCANCODE_TAB: return 15;
        case SDL_SCANCODE_Q: return 16;
        case SDL_SCANCODE_W: return 17;
        case SDL_SCANCODE_E: return 18;
        case SDL_SCANCODE_R: return 19;
        case SDL_SCANCODE_T: return 20;
        case SDL_SCANCODE_Y: return 21;
        case SDL_SCANCODE_U: return 22;
        case SDL_SCANCODE_I: return 23;
        case SDL_SCANCODE_O: return 24;
        case SDL_SCANCODE_P: return 25;
        case SDL_SCANCODE_LEFTBRACKET: return 26;
        case SDL_SCANCODE_RIGHTBRACKET: return 27;
        case SDL_SCANCODE_RETURN: return 28;
        case SDL_SCANCODE_LCTRL: return 29;
        case SDL_SCANCODE_A: return 30;
        case SDL_SCANCODE_S: return 31;
        case SDL_SCANCODE_D: return 32;
        case SDL_SCANCODE_F: return 33;
        case SDL_SCANCODE_G: return 34;
        case SDL_SCANCODE_H: return 35;
        case SDL_SCANCODE_J: return 36;
        case SDL_SCANCODE_K: return 37;
        case SDL_SCANCODE_L: return 38;
        case SDL_SCANCODE_SEMICOLON: return 39;
        case SDL_SCANCODE_APOSTROPHE: return 40;
        case SDL_SCANCODE_GRAVE: return 41;
        case SDL_SCANCODE_LSHIFT: return 42;
        case SDL_SCANCODE_BACKSLASH: return 43;
        case SDL_SCANCODE_Z: return 44;
        case SDL_SCANCODE_X: return 45;
        case SDL_SCANCODE_C: return 46;
        case SDL_SCANCODE_V: return 47;
        case SDL_SCANCODE_B: return 48;
        case SDL_SCANCODE_N: return 49;
        case SDL_SCANCODE_M: return 50;
        case SDL_SCANCODE_COMMA: return 51;
        case SDL_SCANCODE_PERIOD: return 52;
        case SDL_SCANCODE_SLASH: return 53;
        case SDL_SCANCODE_RSHIFT: return 54;
        case SDL_SCANCODE_KP_MULTIPLY: return 55;
        case SDL_SCANCODE_LALT: return 56;
        case SDL_SCANCODE_SPACE: return 57;
        case SDL_SCANCODE_CAPSLOCK: return 58;
        case SDL_SCANCODE_F1: return 59;
        case SDL_SCANCODE_F2: return 60;
        case SDL_SCANCODE_F3: return 61;
        case SDL_SCANCODE_F4: return 62;
        case SDL_SCANCODE_F5: return 63;
        case SDL_SCANCODE_F6: return 64;
        case SDL_SCANCODE_F7: return 65;
        case SDL_SCANCODE_F8: return 66;
        case SDL_SCANCODE_F9: return 67;
        case SDL_SCANCODE_F10: return 68;
        case SDL_SCANCODE_NUMLOCKCLEAR: return 69;
        case SDL_SCANCODE_SCROLLLOCK: return 70;
        case SDL_SCANCODE_KP_7: return 71;
        case SDL_SCANCODE_KP_8: return 72;
        case SDL_SCANCODE_KP_9: return 73;
        case SDL_SCANCODE_KP_MINUS: return 74;
        case SDL_SCANCODE_KP_4: return 75;
        case SDL_SCANCODE_KP_5: return 76;
        case SDL_SCANCODE_KP_6: return 77;
        case SDL_SCANCODE_KP_PLUS: return 78;
        case SDL_SCANCODE_KP_1: return 79;
        case SDL_SCANCODE_KP_2: return 80;
        case SDL_SCANCODE_KP_3: return 81;
        case SDL_SCANCODE_KP_0: return 82;
        case SDL_SCANCODE_KP_PERIOD: return 83;
        case SDL_SCANCODE_LANG5: return 85;
        case SDL_SCANCODE_NONUSBACKSLASH: return 86;
        case SDL_SCANCODE_F11: return 87;
        case SDL_SCANCODE_F12: return 88;
        case SDL_SCANCODE_INTERNATIONAL1: return 89;
        case SDL_SCANCODE_LANG3: return 90;
        case SDL_SCANCODE_LANG4: return 91;
        case SDL_SCANCODE_INTERNATIONAL4: return 92;
        case SDL_SCANCODE_INTERNATIONAL2: return 93;
        case SDL_SCANCODE_INTERNATIONAL5: return 94;
        case SDL_SCANCODE_KP_ENTER: return 96;
        case SDL_SCANCODE_RCTRL: return 97;
        case SDL_SCANCODE_KP_DIVIDE: return 98;
        case SDL_SCANCODE_SYSREQ: return 99;
        case SDL_SCANCODE_RALT: return 100;
        case SDL_SCANCODE_HOME: return 102;
        case SDL_SCANCODE_UP: return 103;
        case SDL_SCANCODE_PAGEUP: return 104;
        case SDL_SCANCODE_LEFT: return 105;
        case SDL_SCANCODE_RIGHT: return 106;
        case SDL_SCANCODE_END: return 107;
        case SDL_SCANCODE_DOWN: return 108;
        case SDL_SCANCODE_PAGEDOWN: return 109;
        case SDL_SCANCODE_INSERT: return 110;
        case SDL_SCANCODE_DELETE: return 111;
        case SDL_SCANCODE_MUTE: return 113;
        case SDL_SCANCODE_VOLUMEDOWN: return 114;
        case SDL_SCANCODE_VOLUMEUP: return 115;
        case SDL_SCANCODE_POWER: return 116;
        case SDL_SCANCODE_KP_EQUALS: return 117;
        case SDL_SCANCODE_KP_PLUSMINUS: return 118;
        case SDL_SCANCODE_PAUSE: return 119;
        case SDL_SCANCODE_KP_COMMA: return 121;
        case SDL_SCANCODE_LANG1: return 122;
        case SDL_SCANCODE_LANG2: return 123;
        case SDL_SCANCODE_INTERNATIONAL3: return 124;
        case SDL_SCANCODE_LGUI: return 125;
        case SDL_SCANCODE_RGUI: return 126;
        case SDL_SCANCODE_APPLICATION: return 127;
        case SDL_SCANCODE_STOP: return 128;
        case SDL_SCANCODE_AGAIN: return 129;
        case SDL_SCANCODE_UNDO: return 131;
        case SDL_SCANCODE_COPY: return 133;
        case SDL_SCANCODE_PASTE: return 135;
        case SDL_SCANCODE_FIND: return 136;
        case SDL_SCANCODE_CUT: return 137;
        case SDL_SCANCODE_HELP: return 138;
        case SDL_SCANCODE_MENU: return 139;
        case SDL_SCANCODE_CALCULATOR: return 140;
        case SDL_SCANCODE_SLEEP: return 142;
        case SDL_SCANCODE_APP1: return 148;
        case SDL_SCANCODE_APP2: return 149;
        case SDL_SCANCODE_WWW: return 150;
        case SDL_SCANCODE_MAIL: return 155;
        case SDL_SCANCODE_AC_BOOKMARKS: return 156;
        case SDL_SCANCODE_COMPUTER: return 157;
        case SDL_SCANCODE_AC_BACK: return 158;
        case SDL_SCANCODE_AC_FORWARD: return 159;
        case SDL_SCANCODE_EJECT: return 161;
        case SDL_SCANCODE_AUDIONEXT: return 163;
        case SDL_SCANCODE_AUDIOPLAY: return 164;
        case SDL_SCANCODE_AUDIOPREV: return 165;
        case SDL_SCANCODE_AUDIOSTOP: return 166;
#if SDL_VERSION_ATLEAST(2, 0, 6)
        case SDL_SCANCODE_AUDIOREWIND: return 168;
#endif
        case SDL_SCANCODE_AC_HOME: return 172;
        case SDL_SCANCODE_AC_REFRESH: return 173;
        case SDL_SCANCODE_KP_LEFTPAREN: return 179;
        case SDL_SCANCODE_KP_RIGHTPAREN: return 180;
        case SDL_SCANCODE_F13: return 183;
        case SDL_SCANCODE_F14: return 184;
        case SDL_SCANCODE_F15: return 185;
        case SDL_SCANCODE_F16: return 186;
        case SDL_SCANCODE_F17: return 187;
        case SDL_SCANCODE_F18: return 188;
        case SDL_SCANCODE_F19: return 189;
        case SDL_SCANCODE_F20: return 190;
        case SDL_SCANCODE_F21: return 191;
        case SDL_SCANCODE_F22: return 192;
        case SDL_SCANCODE_F23: return 193;
        case SDL_SCANCODE_F24: return 194;
#if SDL_VERSION_ATLEAST(2, 0, 6)
        case SDL_SCANCODE_AUDIOFASTFORWARD: return 208;
#endif
        case SDL_SCANCODE_AC_SEARCH: return 217;
        case SDL_SCANCODE_ALTERASE: return 222;
        case SDL_SCANCODE_CANCEL: return 223;
        case SDL_SCANCODE_BRIGHTNESSDOWN: return 224;
        case SDL_SCANCODE_BRIGHTNESSUP: return 225;
        case SDL_SCANCODE_DISPLAYSWITCH: return 227;
        case SDL_SCANCODE_KBDILLUMTOGGLE: return 228;
        case SDL_SCANCODE_KBDILLUMDOWN: return 229;
        case SDL_SCANCODE_KBDILLUMUP: return 230;
        default: return 0;
    }
}

/* release all pressed keys */
static void sdl_reset_keys(VirtMachine *m)
{
    int i;

    for(i = 1; i <= KEYCODE_MAX; i++) {
        if (key_pressed[i]) {
            vm_send_key_event(m, FALSE, i);
            key_pressed[i] = FALSE;
        }
    }
}

static void sdl_handle_key_event(const SDL_KeyboardEvent *ev, VirtMachine *m)
{
    int keycode, keypress;

    keycode = sdl_get_keycode(ev);
    if (keycode) {
        if (keycode == 0x3a || keycode ==0x45) {
            /* SDL does not generate key up for numlock & caps lock */
            vm_send_key_event(m, TRUE, keycode);
            vm_send_key_event(m, FALSE, keycode);
        } else {
            keypress = (ev->type == SDL_KEYDOWN);
            if (keycode <= KEYCODE_MAX)
                key_pressed[keycode] = keypress;
            vm_send_key_event(m, keypress, keycode);
        }
    } else if (ev->type == SDL_KEYUP) {
        /* workaround to reset the keyboard state (used when changing
           desktop with ctrl-alt-x on Linux) */
        sdl_reset_keys(m);
    }
}

static void sdl_send_mouse_event(VirtMachine *m, int x1, int y1,
                                 int dz, int state, BOOL is_absolute)
{
    int buttons, x, y;

    buttons = 0;
    if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
        buttons |= (1 << 0);
    if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
        buttons |= (1 << 1);
    if (state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
        buttons |= (1 << 2);
    if (is_absolute) {
        x = (x1 * 32768) / window_width;
        y = (y1 * 32768) / window_height;
    } else {
        x = x1;
        y = y1;
    }
    vm_send_mouse_event(m, x, y, dz, buttons);
}

static void sdl_handle_mouse_motion_event(const SDL_Event *ev, VirtMachine *m)
{
    BOOL is_absolute = vm_mouse_is_absolute(m);
    int x, y;
    if (is_absolute) {
        x = ev->motion.x;
        y = ev->motion.y;
    } else {
        x = ev->motion.xrel;
        y = ev->motion.yrel;
    }
    sdl_send_mouse_event(m, x, y, 0, ev->motion.state, is_absolute);
}

static void sdl_handle_mouse_button_event(const SDL_Event *ev, VirtMachine *m)
{
    BOOL is_absolute = vm_mouse_is_absolute(m);
    int state, dz;

    dz = 0;
    if (ev->type == SDL_MOUSEWHEEL)
        dz = ev->wheel.y;

    state = SDL_GetMouseState(NULL, NULL);
    /* just in case */
    if (ev->type == SDL_MOUSEBUTTONDOWN)
        state |= SDL_BUTTON(ev->button.button);
    else
        state &= ~SDL_BUTTON(ev->button.button);

    if (is_absolute) {
        sdl_send_mouse_event(m, ev->button.x, ev->button.y,
                             dz, state, is_absolute);
    } else {
        sdl_send_mouse_event(m, 0, 0, dz, state, is_absolute);
    }
}

void sdl_refresh(VirtMachine *m)
{
    SDL_Event ev_s, *ev = &ev_s;

    if (!m->fb_dev)
        return;

    sdl_update_fb_texture(m->fb_dev);

    int dirty = 0;
    m->fb_dev->refresh(m->fb_dev, sdl_update, &dirty);

    if (dirty) {
        SDL_UpdateTexture(fb_texture, NULL,
                          m->fb_dev->fb_data,
                          m->fb_dev->stride);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, fb_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    while (SDL_PollEvent(ev)) {
        switch (ev->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdl_handle_key_event(&ev->key, m);
            break;
        case SDL_MOUSEMOTION:
            sdl_handle_mouse_motion_event(ev, m);
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            sdl_handle_mouse_button_event(ev, m);
            break;
        case SDL_QUIT:
            exit(0);
        }
    }
}

static void sdl_hide_cursor(void)
{
    uint8_t data = 0;
    sdl_cursor_hidden = SDL_CreateCursor(&data, &data, 8, 1, 0, 0);
    SDL_ShowCursor(1);
    SDL_SetCursor(sdl_cursor_hidden);
}

void sdl_init(int width, int height)
{
    window_width = width;
    window_height = height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE)) {
        fprintf(stderr, "Could not initialize SDL - exiting\n");
        exit(1);
    }

    int result = SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
    if (result == -1) {
        fprintf(stderr, "Could not create SDL window\n");
        exit(1);
    }

    SDL_SetWindowTitle(window, "TinyEMU");

    sdl_hide_cursor();
}
