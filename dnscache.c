/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2020 Mateusz Viste
 */

#include <time.h>
#include <string.h>

#include "config.h"
#include "dnscache.h"


struct dnscache_t {
  char host[DNS_MAXHOSTLEN + 1];
  char addr[64];
  time_t inserttime;
};

static struct dnscache_t dnscache_table[DNS_MAXENTRIES];


/* fills ipaddr with cached IP for host, returns 0 on success */
int dnscache_ask(char *ipaddr, const char *host) {
  int x;
  time_t oldlimit = time(NULL) - DNS_CACHETIME;
  for (x = 0; x < DNS_MAXENTRIES; x++) {
    if (dnscache_table[x].inserttime < oldlimit) continue; /* expired entry */
    if (strcasecmp(host, dnscache_table[x].host) == 0) {
      strcpy(ipaddr, dnscache_table[x].addr);
      return(0);
    }
  }
  return(-1);
}


/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, const char *ipaddr) {
  int x, oldest = 0;
  if (strlen(host) > DNS_MAXHOSTLEN) return; /* if host len too long, abort */
  /* find the best slot for storing the new entry (either oldest slot or the one for same entry)*/
  for (x = 0; x < DNS_MAXENTRIES; x++) {
    if (dnscache_table[x].inserttime < dnscache_table[oldest].inserttime) oldest = x; /* remember the oldest entry */
    if (dnscache_table[x].inserttime > 0) { /* check if it's an already known host */
      if (strcasecmp(dnscache_table[x].host, host) == 0) {
        oldest = x;
        break;
      }
    }
  }
  /* replace the chosen slot with new entry */
  dnscache_table[oldest].inserttime = time(NULL);
  strcpy(dnscache_table[oldest].host, host);
  strcpy(dnscache_table[oldest].addr, ipaddr);
}
