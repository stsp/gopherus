/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#include "startpg.h"
#include "idoc/idoc.h"
#include "idoc/idict.h"

static int idoc_unpack(char *buf, unsigned short bufsz, const unsigned char *idoc, unsigned short bytelen) {
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
int loadembeddedstartpage(char *buffer, unsigned long buffer_max, const char *token) {
  int res;
  if (buffer_max > 0xffff) buffer_max = 0xffff; /* avoid 16 bit clipping */
  if (token[0] == 'm') { /* manual */
    res = idoc_unpack(buffer, buffer_max, idoc_manual, sizeof(idoc_manual));
  } else { /* welcome screen */
    res = idoc_unpack(buffer, buffer_max, idoc_welcome, sizeof(idoc_welcome));
  }
  return(res);
}
