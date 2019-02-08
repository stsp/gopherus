/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 *
 * Defines compile-time configurations:
 *
 * DNS_MAXENTRIES  - max amount of DNS entries to keep in cache
 * DNS_MAXHOSTLEN  - max hostname's length (bytes) for DNS cache
 * DNS_CACHETIME   - how long (seconds) to keep the DNS entries in cache
 * MAXALLOWEDCACHE - history cache size (must be at least PAGEBUFSZ bytes)
 * PAGEBUFSZ       - page buffer size (max size of a single page, bytes)
 * MAXMENULINES    - max amount of lines in a gopher menu page
 * MAXALLOWEDCACHE - max size of cacheable page (bytes)
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifndef DNS_MAXENTRIES
#define DNS_MAXENTRIES 16l
#endif

#ifndef DNS_MAXHOSTLEN
#define DNS_MAXHOSTLEN 31l
#endif

#ifndef DNS_CACHETIME
#define DNS_CACHETIME 120l
#endif

#ifndef PAGEBUFSZ
#define PAGEBUFSZ 1024l*1024
#endif

#ifndef MAXMENULINES
#define MAXMENULINES 1024
#endif

#ifndef MAXALLOWEDCACHE
#define MAXALLOWEDCACHE 1024l*1024*2
#endif

#endif
