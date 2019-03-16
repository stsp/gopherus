/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#include <stdio.h>

#include "readflin.h"

/* reads a single line from file descriptor f and fills memory at b with it
 * (including trailing \n).
 * returns length of line (still including the \n terminator) or 0 on EOF */
unsigned short readfline(char *b, unsigned short blen, FILE *f) {
  unsigned short l = 0;
  int c;
  for (;;) {
    if (l >= blen) return(0);
    c = fgetc(f);
    if (c < 0) break;
    b[l++] = c;
    if (c == '\n') break;
  }
  /* normalize to unix ending if CR/LF ending found */
  if ((l >= 2) && (b[l - 1] == '\n') && (b[l - 2] == '\r')) {
    b[l - 2] = '\n';
    l--;
  }
  /* Sarah Connor called */
  b[l] = 0;
  return(l);
}
