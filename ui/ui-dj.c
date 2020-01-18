/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013-2016
 *
 * Provides all UI functions used by Gopherus, based on DJGPP conio facilities.
 */

#include <conio.h>
#include <pc.h>    /* ScreenRows() */

#include "ui.h"  /* include self for control */


/* inits the UI subsystem */
int ui_init(void) {
  return(0);
}


void ui_close(void) {
}


int ui_getrowcount(void) {
 return(ScreenRows());
}


int ui_getcolcount(void) {
 return(ScreenCols());
}


void ui_cls(void) {
  clrscr();
}


void ui_puts(const char *str) {
  cprintf("%s\r\n", str);
}


void ui_locate(int y, int x) {
  ScreenSetCursor(y, x);
}


void ui_putchar(uint32_t c, int attr, int x, int y) {
  if (c > 255) c = '.';
  ScreenPutChar(c, attr, x, y);
}


int ui_getkey(void) {
  return(getkey());
}


int ui_kbhit(void) {
  return(kbhit());
}


void ui_cursor_show(void) {
  _setcursortype(_NORMALCURSOR);
}


void ui_cursor_hide(void) {
  _setcursortype(_NOCURSOR);
}


void ui_refresh(void) {
  /* nothing here, we draw directly to video memory already */
}
