/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#include <time.h>
#include <string.h>

#include "config.h"
#include "dnscache.h"


struct dnscache_type4 {
  char host[DNS_MAXHOSTLEN + 1];
  unsigned long addr;
  time_t inserttime;
};

static struct dnscache_type4 dnscache_table4[DNS_MAXENTRIES];


/* returns the ip addr if host found in cache, 0 otherwise */
unsigned long dnscache_ask(const char *host) {
  int x;
  time_t curtime = time(NULL);
  for (x = 0; x < DNS_MAXENTRIES; x++) {
    if ((curtime - dnscache_table4[x].inserttime) < DNS_CACHETIME) {
      if (strcasecmp(host, dnscache_table4[x].host) == 0) return(dnscache_table4[x].addr);
    }
  }
  return(0);
}


/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, unsigned long ipaddr) {
  int x, oldest = 0;
  if (strlen(host) > DNS_MAXHOSTLEN) return; /* if host len too long, abort */
  for (x = 0; x < DNS_MAXENTRIES; x++) {
    if (dnscache_table4[x].inserttime < dnscache_table4[oldest].inserttime) oldest = x; /* remember the oldest entry */
    if (dnscache_table4[x].inserttime > 0) { /* check if it's an already known host */
      if (strcasecmp(dnscache_table4[x].host, host) == 0) {
        oldest = x;
        break;
      }
    }
  }
  /* replace the oldest entry */
  dnscache_table4[oldest].inserttime = time(NULL);
  strcpy(dnscache_table4[oldest].host, host);
  dnscache_table4[oldest].addr = ipaddr;
}
