/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <fcntl.h>
#include <i86.h>
#include <io.h>     /* _chsize() */
#include <string.h>

#include "fs.h"

/* fetch directory where the program resides, and return its length. result
 * string is never longer than 128 (incl. the null terminator), and it is
 * always terminated with a backslash separator, unless it is an empty string */
static int exepath(char *result) {
  char far *psp, far *env;
  unsigned int envseg, pspseg, x, i;
  int lastsep;
  union REGS regs;
  /* get the PSP segment */
  regs.h.ah = 0x62;
  int86(0x21, &regs, &regs),
  pspseg = regs.x.bx;
  /* compute a far pointer that points to the top of PSP */
  psp = MK_FP(pspseg, 0);
  /* fetch the segment address of the environment */
  envseg = psp[0x2D];
  envseg <<= 8;
  envseg |= psp[0x2C];
  /* compute the env pointer */
  env = MK_FP(envseg, 0);
  /* skip all environment variables */
  x = 0;
  for (;;) {
    x++;
    if (env[x] == 0) { /* end of variable */
      x++;
      if (env[x] == 0) break; /* end of list */
    }
  }
  x++;
  /* read the WORD that indicates string that follow */
  if (env[x] < 1) {
    result[0] = 0;
    return(0);
  }
  x += 2;
  /* else copy the EXEPATH to our return variable, and truncate after last '\' */
  lastsep = -1;
  for (i = 0;; i++) {
    result[i] = env[x++];
    if (result[i] == '\\') lastsep = i;
    if (result[i] == 0) break; /* end of string */
    if (i >= 126) break;       /* this DOS string should never go beyond 127 chars! */
  }
  result[lastsep + 1] = 0;
  return(lastsep + 1);
}

char *bookmarks_getfname(const char *argv0) {
  static char b[128 + 12];
  int plen;
  plen = exepath(b);
  memcpy(b + plen, "GOPHERUS.BKM", 13);
  return(b);
}

void filetrunc(const char *fname, long sz) {
  int handle;
  handle = open(fname, O_RDWR | O_BINARY);
  if (handle == -1) return;
  _chsize(handle, sz);
  close(handle);
}
