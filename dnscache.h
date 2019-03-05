/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */


#ifndef dnscache_h_sentinel
#define dnscache_h_sentinel

/* returns the ip addr if host found in cache, 0 otherwise */
unsigned long dnscache_ask(const char *host);

/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, unsigned long ipaddr);

#endif
