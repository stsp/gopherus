/*
 * idocgen reads a 7-bit ascii file and computes an idoc C array.
 *
 * Copyright (C) Mateusz Viste, 2019
 */

#include <stdio.h>
#include <malloc.h>

#include "idict.h"

static void help(void) {
    printf("idocgen reads a 7-bit ascii file and computes an idoc C array\n"
           "\n"
           "usage: idoc file.txt varname >> file.h\n");
}


static int gramlookup(const char *s) {
  unsigned short i;
  for (i = 0; i < sizeof(gramdict); i += 4) {
    if (gramdict[i] != s[0]) continue;
    if (gramdict[i+1] != s[1]) continue;
    if (gramdict[i+2] != s[2]) continue;
    if (gramdict[i+3] != s[3]) continue;
    return(128 | (i >> 2));
  }
  return(-1);
}


static void processfile(const char *varname, const char *fdata, unsigned short fdatalen) {
  unsigned short i, ri = 0;

  printf("const unsigned char %s[] = {", varname);

  for (i = 0; i < fdatalen; i++) {
    int c;
    if (i > 0) printf(",");
    if ((ri & 15) == 0) printf("\n");
    c = fdata[i];
    /* look for a gram */
    if ((i + 3) < fdatalen) {
      int g;
      g = gramlookup(fdata + i);
      if (g >= 0) {
        c = g;
        i += 3;
      }
    }
    printf("%3d", c);
    ri++;
  }
  printf("};\n");
}

int main(int argc, char **argv) {
  FILE *fp;
  char *fdata, *fname, *varname;
  unsigned short fdatalen;

  if (argc != 3) {
    help();
    return(1);
  }

  fname = argv[1];
  varname = argv[2];

  fdata = malloc(0xffff);
  if (fdata == NULL) {
    printf("ERR: out of memory\n");
    return(1);
  }

  fp = fopen(fname, "rb");
  if (fp == NULL) {
    free(fdata);
    printf("ERR: failed to open '%s'\n", fname);
    return(1);
  }

  /* read content of file into fdata, up to 64K */
  fdatalen = fread(fdata, 1, 0xffff, fp);
  fclose(fp);

  processfile(varname, fdata, fdatalen);

  return(0);
}
