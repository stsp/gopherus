/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019-2022 Mateusz Viste
 *
 * mdr_* functions are borrowed from the MDR library:
 * http://mdr.osdn.io
 */

#include <fcntl.h>
#include <i86.h>
#include <io.h>     /* _chsize() */
#include <string.h>

#include "fs.h"


/* returns a far pointer to the current process PSP structure */
static void far *mdr_dos_psp(void) {
  unsigned short segm = 0;
  _asm {
    push ax
    push bx
    mov ah, 0x62 /* DOS 3.0+ - get current PSP address (returned in BX) */
    int 0x21
    mov segm, bx
    pop bx
    pop ax
  }
  return(MK_FP(segm, 0));
}


/* returns a far pointer to the environment block of the current process */
static char far *mdr_dos_env(void) {
  unsigned short far *psp;
  /* environment's segment is at offset 44 of the PSP (and it's a WORD) */
  psp = mdr_dos_psp();
  return(MK_FP(psp[22], 0));
}


/* fetches directory where the program was loaded from and return its length.
 * path string is never longer than 128 (incl. the null terminator) and it is
 * always terminated with a backslash separator, unless it is an empty string */
static unsigned char mdr_dos_exepath(char *path) {
  char far *env;
  unsigned int i;
  int lastsep;

  /* compute the env pointer */
  env = mdr_dos_env();

  /* skip all environment variables */
  for (;;) {
    while (*env != 0) env++; /* go to end of variable */
    env++;
    if (*env == 0) break; /* end of list */
  }

  /* read the WORD that indicates the amount of strings that follow - must
   * be at least 1 since the full prog name is the first one */
  env++;
  if (*((unsigned short far *)env) == 0) {
    path[0] = 0;
    return(0);
  }

  /* copy the EXEPATH to result and truncate after last '\' */
  env += 2;
  lastsep = -1;
  for (i = 0;; i++) {
    path[i] = *env;
    if (path[i] == '\\') lastsep = i;
    if (path[i] == 0) break; /* end of string */
    if (i >= 126) {
      lastsep = -1;
      break;     /* this DOS string should never go beyond 127 chars! */
    }
    env++;
  }
  lastsep++;
  path[lastsep] = 0;
  return(lastsep);
}


char *bookmarks_getfname(char *s, size_t ssz) {
  unsigned char plen;
  if (ssz < 128 + 13) return(NULL);
  plen = mdr_dos_exepath(s);
  memcpy(s + plen, "GOPHERUS.BKM", 13);
  return(s);
}


char *config_getfname(char *s, size_t ssz) {
  unsigned char plen;
  if (ssz < 128 + 13) return(NULL);
  plen = mdr_dos_exepath(s);
  memcpy(s + plen, "GOPHERUS.CFG", 13);
  return(s);
}


void filetrunc(const char *fname, long sz) {
  int handle;
  handle = open(fname, O_RDWR | O_BINARY);
  if (handle == -1) return;
  _chsize(handle, sz);
  close(handle);
}
