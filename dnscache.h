/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2020 Mateusz Viste
 */


#ifndef dnscache_h_sentinel
#define dnscache_h_sentinel

/* fills ipaddr with cached IP for host, returns 0 on success */
int dnscache_ask(char *ipaddr, const char *host);

/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, const char *ipaddr);

#endif
