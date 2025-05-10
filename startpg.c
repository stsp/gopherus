/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2022 Mateusz Viste
 */

#include <stdio.h>

#include "config.h"
#include "idoc/idoc.h"
#include "idoc/idict.h"
#include "fs/fs.h"
#include "readflin.h"

#include "startpg.h"


static size_t idoc_unpack(char *buf, size_t bufsz, const unsigned char *idoc, size_t bytelen) {
  size_t i = 0, y = 0;
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
size_t loadembeddedstartpage(char *buffer, size_t buffer_max, const char *token, const char *bookmarksfile) {
  size_t res;
  unsigned short favcount = 0;
  if (token[0] == 'm') { /* manual */
    res = idoc_unpack(buffer, buffer_max, idoc_manual, sizeof(idoc_manual));
  } else { /* welcome screen */
    FILE *f;
    res = idoc_unpack(buffer, buffer_max, idoc_welcome, sizeof(idoc_welcome));
    /* insert bookmarks here (if any) */
    f = fopen(bookmarksfile, "rb");
    if (f != NULL) {
      for (;;) {
        size_t r;
        if ((res + 2ul * (MAXHOSTLEN + MAXSELLEN + 8ul)) > buffer_max) break;
        r = readfline(buffer + res, 2ul * (MAXHOSTLEN + MAXSELLEN + 8ul), f);
        if (r == 0) break;
        if (r == 1) continue; /* skip empty lines */
        r--; /* readfline() returns line len - 1 */
        res += r;
        buffer[res++] = '\n';
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
