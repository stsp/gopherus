/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "fs.h"

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(const char *argv0) {
  static char r[512];
  snprintf(r, sizeof(r), "%s/Gopherus", getenv("APPDATA"));
  CreateDirectory(r, NULL);
  snprintf(r, sizeof(r), "%s/Gopherus/gopherus.bkm", getenv("APPDATA"));
  return(r);
}
