/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013-2020
 *
 * Provides all UI functions used by Gopherus, wrapped around a virtual
 * terminal emulated via SDL2 calls.
 */

#include <stdio.h>  /* puts() */
#include <stdlib.h> /* atexit() */

#include <SDL2/SDL.h>
#include "ui.h"    /* include self for control */
#include "../sdlfiles/ascii.h" /* ascii fonts */
#include "../sdlfiles/icon.h"  /* a raw pixel array with gopheurs icon */

static int cursorx, cursory;
static SDL_Renderer *renderer;
static SDL_Texture *screen;
static int cursorstate = 1;

#define SCREENWIDTH 80
#define SCREENHEIGHT 30

/* a screen buffer that is updated by the application, and put into the
 * renderer only at refresh time */
static unsigned short screenbuffer[SCREENHEIGHT][SCREENWIDTH];


int ui_init(void) {
  SDL_Window *window;
  SDL_Surface *icosurface;
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Gopherus", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    SDL_Quit();
    puts("window fail");
    puts(SDL_GetError());
    return(-1);
  }
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) {
    SDL_Quit();
    puts("renderer fail");
    puts(SDL_GetError());
    return(-1);
  }
  screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
  if (screen == NULL) {
    SDL_Quit();
    return(-2);
  }
  SDL_SetWindowMinimumSize(window, 640, 480);

  /* load the gopherus icon to titlebar */
  icosurface = SDL_CreateRGBSurfaceFrom(icopixels,64,64,16,64*2,0x0f00,0x00f0,0x000f,0xf000);
  SDL_SetWindowIcon(window, icosurface);
  SDL_FreeSurface(icosurface);
  /* enable unicode 'textinput' events */
  SDL_StartTextInput();
  /* make sur to close SDL properly at exit time */
  atexit(SDL_Quit);
  return(0);
}


void ui_close(void) {
  SDL_Quit();
}


int ui_getrowcount(void) {
  return(SCREENHEIGHT);
}


int ui_getcolcount(void) {
  return(SCREENWIDTH);
}


void ui_cls(void) {
  int x, y;
  for (y = 0; y < SCREENHEIGHT; y++) {
    for (x = 0; x < SCREENWIDTH; x++) {
      ui_putchar(' ', 0, x, y);
    }
  }
  ui_refresh();
}


void ui_puts(const char *str) {
  puts(str);
}


void ui_locate(int y, int x) {
  cursorx = x;
  cursory = y;
}


void ui_putchar(uint32_t c, int attr, int x, int y) {
  if ((x >= SCREENWIDTH) || (y >= SCREENHEIGHT) || (x < 0) || (y < 0)) return;
  if (c > 255) c = '.';
  screenbuffer[y][x] = (attr << 8) | (unsigned char)c;
}


int ui_getkey(void) {
  SDL_Event event;
  for (;;) {
    if (SDL_WaitEvent(&event) == 0) return(0); /* block until an event is received */
    if (event.type == SDL_KEYDOWN) { /* I'm mostly interested in key presses */
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE: /* Escape */
          return(0x1B);
        case SDLK_BACKSPACE: /* Backspace */
          return(0x08);
        case SDLK_TAB:    /* TAB */
          return(0x09);
        case SDLK_RETURN: /* ENTER */
        case SDLK_KP_ENTER:
          return(0x0D);
        case SDLK_F1:     /* F1 */
          return(0x13B);
        case SDLK_F2:     /* F2 */
          return(0x13C);
        case SDLK_F3:     /* F3 */
          return(0x13D);
        case SDLK_F4:     /* F4 */
          return(0x13E);
        case SDLK_F5:     /* F5 */
          return(0x13F);
        case SDLK_F6:     /* F6 */
          return(0x140);
        case SDLK_F7:     /* F7 */
          return(0x141);
        case SDLK_F8:     /* F8 */
          return(0x142);
        case SDLK_F9:     /* F9 */
          return(0x143);
        case SDLK_F10:    /* F10 */
          return(0x144);
        case SDLK_HOME:   /* HOME */
          return(0x147);
        case SDLK_UP:     /* UP */
        case SDLK_KP_8:
          return(0x148);
        case SDLK_PAGEUP: /* PGUP */
          return(0x149);
        case SDLK_LEFT:   /* LEFT */
        case SDLK_KP_4:
          return(0x14B);
        case SDLK_RIGHT:  /* RIGHT */
        case SDLK_KP_6:
          return(0x14D);
        case SDLK_END:    /* END */
          return(0x14F);
        case SDLK_DOWN:   /* DOWN */
        case SDLK_KP_2:
          return(0x150);
        case SDLK_PAGEDOWN: /* PGDOWN */
          return(0x151);
        case SDLK_DELETE: /* DEL */
          return(0x153);
        default: break;   /* ignore anything else */
          break;
      }
    } else if (event.type == SDL_TEXTINPUT) {
      if (event.text.text[0] < 127) {
        if ((SDL_GetModState() & KMOD_ALT) != 0) return(event.text.text[0] | 0x100);
        return(event.text.text[0]);
      }
    } else if (event.type == SDL_QUIT) {
      return(0xFF);
    } else if (event.type == SDL_KEYUP) { /* silently drop all key up events */
      continue;
    } else if (event.type == SDL_WINDOWEVENT) { /* I want to redraw screen on window events */
      return(0x00); /* return 'unknown key' */
    } else { /* anything else I ignore, to not flood the application with garbage */
      continue;
    }
  } /* for (;;) */
}


static void flushKeyUpEvents(void) {
  int res;
  SDL_Event event;
  for (;;) { /* silently flush all possible 'KEY UP' events */
    SDL_PumpEvents();
    res = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYUP, SDL_KEYUP);
    if (res < 1) return;
  }
}


int ui_kbhit(void) {
  int res;
  flushKeyUpEvents();  /* silently flush all possible 'KEY UP' events */
  res = SDL_PollEvent(NULL);
  if (res < 0) return(0);
  return(res);
}


void ui_cursor_show(void) {
  cursorstate = 1;
}


void ui_cursor_hide(void) {
  cursorstate = 0;
}


void ui_refresh(void) {
  int x, y, xx, yy, pitch, attr;
  unsigned char c;
  uint32_t *glyphbuff, *screenptr;
  const unsigned long attrpal[16] = {0x000000l, 0x0000AAl, 0x00AA00l, 0x00AAAAl, 0xAA0000l, 0xAA00AAl, 0xAA5500l, 0xAAAAAAl, 0x555555l, 0x5555FFl, 0x55FF55l, 0x55FFFFl, 0xFF5555l, 0xFF55FFl, 0xFFFF55l, 0xFFFFFFl};

  SDL_LockTexture(screen, NULL, (void *)&screenptr, &pitch);

  for (y = 0; y < SCREENHEIGHT; y++) {
    for (x = 0; x < SCREENWIDTH; x++) {
      c = screenbuffer[y][x] & 0xff;
      attr = screenbuffer[y][x] >> 8;
      glyphbuff = screenptr + (y * 4 * pitch + x * 8);

      for (yy = 0; yy < 16; yy++) {
        for (xx = 0; xx < 8; xx++) {
          if ((ascii_font[(c << 4) + yy] & (1 << xx)) != 0) {
            glyphbuff[(7 - xx) + yy * (pitch / 4)] = attrpal[attr & 0x0f];
          } else {
            glyphbuff[(7 - xx) + yy * (pitch / 4)] = attrpal[attr >> 4];
          }
        }
      }
      /* draw a static "cursor" over if cursor is enabled and position is right..
       * this is really clumsy, but it works well enough for now... */
      if ((cursorstate != 0) && (cursorx == x) && (cursory == y)) {
        for (yy = 0; yy < 16; yy++) {
          for (xx = 0; xx < 8; xx++) {
            if ((ascii_font[('_' << 4) + yy] & (1 << xx)) != 0) {
              glyphbuff[(7 - xx) + yy * (pitch / 4)] = attrpal[attr & 0x0f];
            }
          }
        }
      }

    } /* for (x...) */
  } /* for (y...) */

  SDL_UnlockTexture(screen);

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, screen, NULL, NULL);
  SDL_RenderPresent(renderer);
}
