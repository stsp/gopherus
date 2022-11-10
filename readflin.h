/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2022 Mateusz Viste
 */

#ifndef READFLIN_H
#define READFLIN_H

/* reads a single line from file descriptor f and fills memory at b with it
 * (without the \n or \r\n line terminator).
 * returns length of line + 1 or 0 on EOF or when line too long to fit in b.
 * on success (result > 0) the b buffer is guaranteed to be nul-terminated.  */
size_t readfline(char *b, size_t blen, FILE *f);

#endif
