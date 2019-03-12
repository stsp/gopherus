/*
 * fs-related routines used by Gopherus.
 * Copyright (C) 2019 Mateusz Viste
 */

#ifndef gophfs_h
#define gophfs_h

/* returns path and filename of the bookmark file */
char *bookmarks_getfname(const char *argv0);

#endif
