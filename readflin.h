/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#ifndef READFLIN_H
#define READFLIN_H

/* reads a single line from file descriptor f and fills memory at b with it
 * (including trailing \n).
 * returns length of line (still including the \n terminator) or 0 on EOF */
unsigned short readfline(char *b, unsigned short blen, FILE *f);

#endif
