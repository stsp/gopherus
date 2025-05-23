/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) 2013-2022 Mateusz Viste
 *
 * Provides all UI functions used by Gopherus, basing on the ncursesw api.
 */

#define _XOPEN_SOURCE_EXTENDED

#include <locale.h>

#ifdef _WIN32
#include <ncursesw/curses.h>
#else
#include <curses.h> /* relates to ncursesw (include dir set in Makefile) */
#endif

#include <stdio.h> /* this one contains the NULL definition */
#include <string.h>

#include "ui.h"  /* include self for control */


WINDOW *mywindow;


static attr_t getorcreatecolor(int col) {
  static attr_t DOSPALETTE[256] = {0};
  static int lastcolid = 0;
  /* if color doesn't exist yet, create it */
  if (DOSPALETTE[col] == 0) {
    unsigned long DOSCOLORS[16] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE, COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, COLOR_RED,   COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
    if (col & 0x80) {         /* bright background */
      init_pair(++lastcolid, DOSCOLORS[col >> 4], DOSCOLORS[col & 0xf]);
      DOSPALETTE[col] = COLOR_PAIR(lastcolid) | WA_BOLD | WA_REVERSE;
    } else if (col & 0x08) {   /* bright foreground */
      init_pair(++lastcolid, DOSCOLORS[col & 0x0f], DOSCOLORS[col >> 4]);
      DOSPALETTE[col] = COLOR_PAIR(lastcolid) | A_BOLD;
    } else {                  /* no bright nothing */
      init_pair(++lastcolid, DOSCOLORS[col & 0x0f], DOSCOLORS[col >> 4]);
      /* init_pair(col+1, COLOR_BLUE, COLOR_MAGENTA); */
      DOSPALETTE[col] = COLOR_PAIR(lastcolid) | A_NORMAL;
    }
  }

  return(DOSPALETTE[col]);
}


/* inits the UI subsystem */
int ui_init(void) {
  setlocale(LC_ALL, "");
  mywindow = initscr();
  if (mywindow == NULL) return(-1);
  start_color();
  raw();
  noecho();
  keypad(stdscr, TRUE); /* capture arrow keys */
  timeout(100); /* getch blocks for 50ms max */
  set_escdelay(50); /* ESC should wait for 50ms max */
  nonl(); /* allow ncurses to detect KEY_ENTER */
  return(0);
}


void ui_close(void) {
  endwin();
}


int ui_getrowcount(void) {
  return(getmaxy(mywindow));
}


int ui_getcolcount(void) {
  return(getmaxx(mywindow));
}


void ui_cls(void) {
  int x, y;
  int maxrows, maxcols;
  clear();
  getmaxyx(mywindow, maxrows, maxcols);
  attron(0);
  for (y = 0; y < maxrows; y++) {
    for (x = 0; x < maxcols; x++) {
      mvaddch(y, x, ' ');
    }
  }
  attroff(0);
  /* bkgd(COLOR_PAIR(colattr)); */
  move(0,0);
  ui_refresh();
}


void ui_puts(const char *str) {
  puts(str);
}


void ui_locate(int y, int x) {
  wmove(mywindow, y, x);
  ui_refresh();
}


void ui_putchar(uint32_t wchar, int attr, int x, int y) {
  int oldx, oldy;
  cchar_t t;

  memset(&t, 0, sizeof(t));

  /* remember cursor position to restore it afterwards */
  getyx(mywindow, oldy, oldx);

  t.attr = getorcreatecolor(attr);
  t.chars[0] = wchar;
  mvwadd_wch(mywindow, y, x, &t);

  /* restore cursor to its initial location */
  move(oldy, oldx);
}


int ui_getkey(void) {
  int res;

  for (;;) {
    res = wgetch(mywindow);
    if (res == KEY_MOUSE) continue; /* ignore mouse events */
    if (res != ERR) break;          /* ERR means "no input available yet" */
  }

  /* either ESC or ALT+some key */
  if (res == 27) {
    res = wgetch(mywindow);
    if (res == ERR) return(27);
    /* else this is an ALT+something combination */
    switch (res) {
      case 'f': return(0x121);
      case 'h': return(0x123);
      case 'j': return(0x124);
      case 's': return(0x11F);
      case 'u': return(0x116);
    }
  }

  switch (res) {
    case KEY_LEFT:      return(0x14B);
    case KEY_RIGHT:     return(0x14D);
    case KEY_UP:        return(0x148);
    case KEY_DOWN:      return(0x150);
    case KEY_BACKSPACE: return(8);
    case KEY_ENTER:     return(13);
    case KEY_PPAGE:     return(0x149);  /* PAGEUP */
    case KEY_NPAGE:     return(0x151);  /* PGDOWN */
    case KEY_HOME:      return(0x147);
    case KEY_END:       return(0x14F);
    case KEY_DC:        return(0x153);  /* DEL */
    case KEY_F(1):      return(0x13b);
    case KEY_F(2):      return(0x13c);
    case KEY_F(3):      return(0x13d);
    case KEY_F(4):      return(0x13e);
    case KEY_F(5):      return(0x13f);
    case KEY_F(6):      return(0x140);
    case KEY_F(7):      return(0x141);
    case KEY_F(8):      return(0x142);
    case KEY_F(9):      return(0x143);
    case KEY_F(10):     return(0x144);
    default:            return(res); /* return the scancode as-is otherwise */
  }
}


int ui_kbhit(void) {
  int tmp;
  timeout(0);
  tmp = wgetch(mywindow);
  timeout(100);
  if (tmp == ERR) return(0);
  ungetch(tmp);
  return(1);
}


void ui_cursor_show(void) {
  curs_set(1);
}


void ui_cursor_hide(void) {
  curs_set(0);
}


void ui_refresh(void) {
  wrefresh(mywindow);
}
