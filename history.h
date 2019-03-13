/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#ifndef history_h_sentinel
#define history_h_sentinel

struct historytype {
  long cachesize;
  char *selector;
  char *cache;
  struct historytype *next;
  unsigned short port;
  char protocol;
  char itemtype;
  long displaymemory[2];  /* used by some display plugins to remember how the item was displayed. this is always initialized to -1 values */
  char host[1];
};

/* remove the last visited page from history (goes back to the previous one) */
void history_back(struct historytype **history);

/* adds a new node to the history list. Returns 0 on success, non-zero otherwise. */
int history_add(struct historytype **history, char protocol, const char *host, unsigned short port, char itemtype, const char *selector);

/* free cache content past latest maxallowedcache bytes */
void history_cleanupcache(struct historytype *history);

/* flush all history, freeing memory */
void history_flush(struct historytype *history);

#endif
