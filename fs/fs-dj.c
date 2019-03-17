/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <stdio.h>
#include <unistd.h>

char *bookmarks_getfname(const char *argv0) {
  static char b[256];
  snprintf(b, sizeof(b), "%s\\gopherus.bkm", dirname(argv0));
  return(b);
}


void filetrunc(const char *fname, long sz) {
  truncate(fname, sz);
}
