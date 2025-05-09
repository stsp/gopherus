/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2019-2022 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* truncate() */

#include "fs.h"

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(char *s, size_t ssz) {
  snprintf(s, ssz, "%s\\gopherus.bkm", getenv("TEMP"));
  return(s);
}


/* returns path and filename of the config file */
char *config_getfname(char *s, size_t ssz) {
  snprintf(s, ssz, "%s\\gopherus.cnf", getenv("TEMP"));
  return(s);
}


void filetrunc(const char *fname, long sz) {
  truncate(fname, sz);
}
