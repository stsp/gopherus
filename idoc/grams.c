/*
 *
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* return the most common x-gram from fdata. gramlen can be 1, 2, 3 or 4. */
static uint32_t bestgram(const unsigned char *fdata, unsigned short fdatalen, int gramlen, unsigned long *count) {
  struct trigramlist {
    uint32_t trigram;
    unsigned long count;
    struct trigramlist *next;
  };
  struct trigramlist *tlist = NULL;
  struct trigramlist *tnode;
  struct trigramlist *tgreatest;
  int i, t;
  uint32_t res;

  /* count trigrams */
  for (i = 0; i < (fdatalen - gramlen); i++) {
    uint32_t tg;
    tg = 0;
    for (t = 0; t < gramlen; t++) {
      tg <<= 8;
      tg |= fdata[i+t];
      if (fdata[i+t] == 0xff) { /* detect when no place for full gram */
        tg = 0xff;
        break;
      }
    }
    if (tg == 0xff) continue; /* skip when no gram can be computed */
    /* look for same tg */
    for (tnode = tlist; tnode != NULL; tnode = tnode->next) {
      if (tnode->trigram == tg) break;
    }
    /* */
    if (tnode != NULL) {
      tnode->count++;
    } else {
      tnode = calloc(1, sizeof(struct trigramlist));
      if (tnode == NULL) printf("OUT OF MEMORY!\n");
      tnode->next = tlist;
      tnode->count = 1;
      tlist = tnode;
      tlist->trigram = tg;
    }
  }
  if (tlist == NULL) {
    *count = 0;
    return(0);
  }
  /* return the most common one */
  tgreatest = tlist;
  for (tnode = tlist; tnode != NULL; tnode = tnode->next) {
    if (tnode->count > tgreatest->count) tgreatest = tnode;
  }
  *count = tgreatest->count;
  res = tgreatest->trigram;
  /* free the list */
  for (tnode = tlist; tnode != NULL; ) {
    tgreatest = tnode;
    tnode = tnode->next;
    free(tgreatest);
  }
  /* */
  return(res);
}


static void removegram(unsigned char *buf, unsigned short buflen, char *gram, int glen) {
  int i, t;
  for (i = 0; i < (buflen - glen); i++) {
    for (t = 0; t < glen; t++) {
      if (buf[i + t] != gram[t]) break;
    }
    /* blank out gram if match */
    if (t == glen) memset(buf + i, 0xff, glen);
  }
}


static char *sgram(char *s, uint32_t gram, int glen) {
  int i;
  for (i = 0; i < glen; i++) {
    s[(glen - 1) - i] = gram & 0xff;
    gram >>= 8;
  }
  s[i] = 0;
  return(s);
}


int main(int argc, char **argv) {
  FILE *fp;
  long flen, flenorig;
  uint32_t gram;
  unsigned long gramcount;
  unsigned char fbuf[0xffff];
  int i, x, glen;
  long totbytes;
  uint32_t dict[128];
  char sgr[5];

  if (argc != 3) {
    printf("usage: grams file.txt glen\n");
    return(1);
  }

  glen = argv[2][0] - '0';
  if ((glen < 1) || (glen > 4)) {
    printf("ERR: gramlen must be within 1..4\n");
    return(1);
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    printf("ERR: fopen() failure\n");
    return(1);
  }

  flen = fread(fbuf, 1, sizeof(fbuf), fp);
  fclose(fp);
  if (flen < 0) {
    printf("ERR: fread() failure\n");
    return(1);
  }
  flenorig = flen;

  totbytes = 0;
  for (x = 0; x < 128; x++) {
    gram = bestgram(fbuf, flen, glen, &gramcount);
    if (gramcount < 2) break;

    dict[x] = gram;

    sgram(sgr, gram, glen);
    printf("'%s' -> %ld (%02x %02x %02x %02x)\n", sgr, gramcount, sgr[0], sgr[1], sgr[2], sgr[3]);
    totbytes += gramcount * (glen - 1);

    removegram(fbuf, flen, sgr, glen);
  }
  {
    long dictsize = x * glen;
    long realgain = totbytes - dictsize; /* dict size */
    long newsize = flenorig - realgain;
    printf("TOTAL BYTES GAIN: %ld (%ld -> %ld, %ld%%), dict size: %ld (%d grams)\n", realgain, flenorig, newsize, (newsize * 100) / flenorig, dictsize, x);
  }
  printf("DICT:\n");
  printf("const unsigned char gramdict[] = {");
  for (i = 0; i < x; i++) {
    int z, z8;
    uint32_t q = dict[i];
    if (i != 0) printf(",");
    if ((i & 3) == 0) printf("\n  ");
    for (z = glen - 1; z >= 0; z--) {
      z8 = z * 8;
      if (z < (glen - 1)) printf(",");
      printf("%d", (q >> z8) & 0xff);
    }
  }
  printf("};\n");

  return(0);
}
