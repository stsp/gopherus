/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>

#include "fs.h"

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(void) {
  static char r[512];
  snprintf(r, sizeof(r), "%s/.gopherus.bookmarks", getenv("HOME"));
  return(r);
}
