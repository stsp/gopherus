/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013-2018
 *
 * Provides all UI functions used by Gopherus, relying on BIOS and DOS
 */

#include <dos.h>

#include "ui.h"  /* include self for control */

unsigned char far *vmem; /* video memory pointer (beginning of page 0) */
int term_width = 0, term_height = 0;
int cursor_start = 0, cursor_end = 0; /* remember the cursor's shape */
unsigned short videomode = 0;

#if !defined(_M_I86)
#define int86(x,y,z) int386(x,y,z)
#endif
/* inits the UI subsystem */
int ui_init(void) {
  union REGS regs = {0};
  regs.h.ah = 0x0F;  /* get current video mode */
  int86(0x10, &regs, &regs);
  videomode = regs.h.al;
  term_width = regs.h.ah; /* int10,F provides number of columns in AH */
  /* read screen length from BIOS at 0040:0084 */
  term_height = (*(unsigned char far *) MK_FP(0x40, 0x84)) + 1;
  if (term_height < 10) term_height = 25; /* assume 25 rows if weird value */
  /* select the correct VRAM address */
  if (videomode == 7) { /* MDA/HERC mode */
    vmem = MK_FP(0xB000, 0); /* B000:0000 video memory addess */
  } else {
    vmem = MK_FP(0xB800, 0); /* B800:0000 video memory address */
  }
  /* get cursor shape */
  regs.h.ah = 3;
  regs.h.bh = 0;
  int86(0x10, &regs, &regs);
  cursor_start = regs.h.ch;
  cursor_end = regs.h.cl;
  return(0);
}

void ui_close(void) {
}

static void cursor_set(int startscanline, int endscanline) {
  union REGS regs = {0};
  regs.h.ah = 0x01;
  regs.h.al = videomode; /* RBIL says some BIOSes require video mode in AL */
  regs.h.ch = startscanline;
  regs.h.cl = endscanline;
  int86(0x10, &regs, &regs);
}

int ui_getrowcount(void) {
  return(term_height);
}


int ui_getcolcount(void) {
  return(term_width);
}


void ui_cls(void) {
  union REGS regs = {0};
  regs.w.ax = 0x0600;  /* Scroll window up, entire window */
  regs.h.bh = 0x07;    /* Attribute to write to screen */
  regs.h.bl = 0;
  regs.w.cx = 0x0000;  /* Upper left */
  regs.h.dh = term_height - 1;
  regs.h.dl = term_width - 1; /* Lower right */
  int86(0x10, &regs, &regs);
  ui_locate(0, 0);
}


void ui_puts(const char *str) {
  union REGS regs = {0};
  /* display the string one character at a time */
  while (*str != 0) {
    regs.h.ah = 0x02;
    regs.h.dl = *str;
    int86(0x21, &regs, &regs);
    str++;
  }
  /* write a CR/LF pair to screen */
  regs.h.ah = 0x02; /* DOS 1+ - WRITE CHARACTER TO STDOUT */
  regs.h.dl = '\r';
  int86(0x21, &regs, &regs);
  regs.h.ah = 0x02; /* DOS 1+ - WRITE CHARACTER TO STDOUT */
  regs.h.dl = '\n';
  int86(0x21, &regs, &regs);
}


void ui_locate(int row, int column) {
  union REGS regs = {0};
  regs.h.ah = 0x02;
  regs.h.bh = 0;
  regs.h.dh = row;
  regs.h.dl = column;
  int86(0x10, &regs, &regs);
}


void ui_putchar(uint32_t c, int attr, int x, int y) {
  unsigned char far *p;
  if (c > 255) c = '.';
  p = vmem + ((y * term_width + x) << 1);
  *p++ = c;
  *p = attr;
}


int ui_getkey(void) {
  union REGS regs = {0};
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  if (regs.h.al != 0) return(regs.h.al);
  /* extended key, read again */
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  return(0x100 | regs.h.al);
}


int ui_kbhit(void) {
  union REGS regs = {0};
  regs.h.ah = 0x0b; /* DOS 1+ - GET STDIN STATUS */
  int86(0x21, &regs, &regs);
  return(regs.h.al);
}


void ui_cursor_show(void) {
  if (cursor_start == 0) return;
  cursor_set(cursor_start, cursor_end); /* unhide the cursor */
}


void ui_cursor_hide(void) {
  cursor_set(0x2F, 0x0E); /* hide the cursor */
  /* the 'start position' of cursor_set() is a bitfield:
   *
   * Bit(s)  Description     (Table 00013)
   *  7      should be zero
   *  6,5    cursor blink (00=normal, 01=invisible)
   *  4-0    topmost scan line containing cursor
   */
}


void ui_refresh(void) {
  /* nothing here, we draw directly to video memory already */
}
