/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* truncate() */

#include "fs.h"

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(const char *argv0) {
  static char r[512];
  if (argv0 == NULL) r[0] = 0; /* stupid thing for gcc to shut up */
  snprintf(r, sizeof(r), "%s/.gopherus.bookmarks", getenv("HOME"));
  return(r);
}

void filetrunc(const char *fname, long sz) {
  truncate(fname, sz);
}
