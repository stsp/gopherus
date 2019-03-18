/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#include <stdio.h>

#include "config.h"
#include "idoc/idoc.h"
#include "idoc/idict.h"
#include "fs/fs.h"
#include "readflin.h"

#include "startpg.h"


static unsigned short idoc_unpack(char *buf, unsigned short bufsz, const unsigned char *idoc, unsigned short bytelen) {
  unsigned short i = 0, y = 0;
  for (; i < bytelen; i++) {
    if (idoc[i] < 128) {
      buf[y++] = idoc[i];
      if ((y + 1) >= bufsz) break;
    } else {
      if ((y + 5) >= bufsz) break;
      buf[y++] = gramdict[(idoc[i] & 127) << 2];
      buf[y++] = gramdict[((idoc[i] & 127) << 2) + 1];
      buf[y++] = gramdict[((idoc[i] & 127) << 2) + 2];
      buf[y++] = gramdict[((idoc[i] & 127) << 2) + 3];
    }
  }
  buf[y] = 0;
  return(y);
}


/* loads the embedded start page into a memory buffer and returns */
int loadembeddedstartpage(char *buffer, unsigned long buffer_max, const char *token, const char *bookmarksfile) {
  unsigned short res;
  unsigned short favcount = 0;
  if (buffer_max > 0xffff) buffer_max = 0xffff; /* avoid 16 bit clipping */
  if (token[0] == 'm') { /* manual */
    res = idoc_unpack(buffer, buffer_max, idoc_manual, sizeof(idoc_manual));
  } else { /* welcome screen */
    FILE *f;
    res = idoc_unpack(buffer, buffer_max, idoc_welcome, sizeof(idoc_welcome));
    /* insert bookmarks here (if any) */
    f = fopen(bookmarksfile, "rb");
    if (f != NULL) {
      for (;;) {
        unsigned short r;
        if ((res + 2ul * (MAXHOSTLEN + MAXSELLEN + 8ul)) > buffer_max) break;
        r = readfline(buffer + res, 2ul * (MAXHOSTLEN + MAXSELLEN + 8ul), f);
        if (r == 0) break;
        if (r < 3) continue; /* skip empty lines */
        res += r;
        if (favcount < 0xffff) favcount++;
      }
      fclose(f);
    }
    /* */
    if (favcount == 0) {
      res += sprintf(buffer + res, "ino bookmarks defined yet\n");
    }
    /* */
    res += idoc_unpack(buffer + res, buffer_max - res, idoc_welcome2, sizeof(idoc_welcome2));
  }
  return(res);
}
