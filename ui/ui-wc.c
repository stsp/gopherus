/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013-2015
 *
 * Provides all UI functions used by Gopherus, basing on OpenWatcom facilities.
 *
 * THIS CODE IS NOT FUNCTIONAL - PROTOTYPE ONLY!
 */

#include <conio.h>
#include <dos.h>

#include "ui.h"  /* include self for control */

unsigned char far *vmem; /* video memory pointer (beginning of page 0) */
int term_width = 0, term_height = 0;
int cursor_start = 0, cursor_end = 0; /* remember the cursor's shape */


static void cursor_set(int startscanline, int endscanline) {
  union REGS regs;
  regs.h.ah = 0x01;
  regs.h.ch = startscanline;
  regs.h.cl = endscanline;
  int86(0x10, &regs, &regs);
}

int ui_getrowcount(void) {
  /* TODO */
  return(25);
}


int ui_getcolcount(void) {
  /* TODO */
  return(80);
}


void ui_cls(void) {
  /* TODO */
}


void ui_puts(char *str) {
  cprintf("%s\r\n", str);
}


void ui_locate(int row, int column) {
  union REGS regs;
  regs.h.ah = 0x02;
  regs.h.bh = 0;
  regs.h.dh = row;
  regs.h.dl = column;
  int86(0x10, &regs, &regs);
}


void ui_putchar(char c, int attr, int x, int y) {
  unsigned char far *p;
  p = vmem + ((y * term_width + x) << 1);
  *p++ = c;
  *p = attr;
}


int ui_getkey(void) {
  union REGS regs;
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  return(regs.h.al);
}


int ui_kbhit(void) {
  return(kbhit());
}


void ui_cursor_show(void) {
  if (cursor_start == 0) return;
  cursor_set(cursor_start, cursor_end); /* unhide the cursor */
}


void ui_cursor_hide(void) {
  cursor_set(0x0F, 0x0E); /* hide the cursor */
}


void ui_refresh(void) {
  /* nothing here, we draw directly to video memory already */
}
