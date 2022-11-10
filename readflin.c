/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2022 Mateusz Viste
 */

#include <stdio.h>

#include "readflin.h"

/* reads a single line from file descriptor f and fills memory at b with it
 * (without the \n or \r\n line terminator).
 * returns length of line + 1 or 0 on EOF or when line too long to fit in b.
 * on success (result > 0) the b buffer is guaranteed to be nul-terminated.  */
size_t readfline(char *b, size_t blen, FILE *f) {
  size_t l = 0;
  int c;

  for (;;) {
    if (l == blen) return(0);
    c = fgetc(f);
    if (c < 0) {
      if (l == 0) return(0);
      break;
    }
    if (c == '\n') break;
    b[l++] = c;
  }

  /* strip CR trailer if present */
  if ((l > 0) && (b[l - 1] == '\r')) l--;

  /* terminate the string */
  b[l] = 0;
  return(l+1);
}
