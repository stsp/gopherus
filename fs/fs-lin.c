/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>

#include "fs.h"

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(const char *argv0) {
  static char r[512];
  argv0 = argv0; /* for gcc to shut up */
  snprintf(r, sizeof(r), "%s/.gopherus.bookmarks", getenv("HOME"));
  return(r);
}
