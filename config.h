/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 *
 * Defines compile-time configurations:
 *
 * MAXHOSTLEN      - max length (bytes) of a hostname
 * MAXSELLEN       - max length (bytes) of a selector string
 * MAXURLLEN       - max length (bytes) of a URL
 * MAXQUERYLEN     - max length (bytes) of a type 7 query
 * DNS_MAXENTRIES  - max amount of DNS entries to keep in cache
 * DNS_MAXHOSTLEN  - max hostname's length (bytes) for DNS cache
 * DNS_CACHETIME   - how long (seconds) to keep the DNS entries in cache
 * MAXALLOWEDCACHE - history cache size (must be at least PAGEBUFSZ bytes)
 * PAGEBUFSZ       - page buffer size (max size of a single page, bytes)
 * MAXMENULINES    - max amount of lines in a gopher menu page
 * MAXALLOWEDCACHE - max size of cacheable page (bytes)
 * NOLFN           - environment is assumed to be 8+3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* max length (bytes) of a hostname */
#ifndef MAXHOSTLEN
#define MAXHOSTLEN 64
#endif

/* max length (bytes) of a selector string */
#ifndef MAXSELLEN
#define MAXSELLEN 256
#endif

/* max length (bytes) of a URL */
#ifndef MAXURLLEN
#define MAXURLLEN 256
#endif

/* max length (bytes) of a type 7 query */
#ifndef MAXQUERYLEN
#define MAXQUERYLEN 256
#endif

/* size of the DNS cache, in entries */
#ifndef DNS_MAXENTRIES
#define DNS_MAXENTRIES 16l
#endif

/* maximum host len handled by the DNS cache */
#ifndef DNS_MAXHOSTLEN
#define DNS_MAXHOSTLEN 31l
#endif

#ifndef DNS_CACHETIME
#define DNS_CACHETIME 120l
#endif

/* max size of a displayed resource (in bytes) */
#ifndef PAGEBUFSZ
#define PAGEBUFSZ 1024l*1024
#endif

/* max amount of lines displayed in a menu */
#ifndef MAXMENULINES
#define MAXMENULINES 4096
#endif

/* max size of cached objects (must be at least PAGEBUFSZ big) */
#ifndef MAXALLOWEDCACHE
#define MAXALLOWEDCACHE 1024l*1024*2
#endif

#endif
