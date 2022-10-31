/*
 * fs-related routines used by Gopherus.
 * Copyright (C) 2019-2022 Mateusz Viste
 */

#ifndef gophfs_h
#define gophfs_h

/* fills s with path and filename of the bookmark file, returns s on success, NULL on error */
char *bookmarks_getfname(char *s, size_t ssz);

/* truncates file fname to sz bytes */
void filetrunc(const char *fname, long sz);

#endif
