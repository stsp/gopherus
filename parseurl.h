/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2020 Mateusz Viste
 */

#ifndef parseurl_h_sentinel
  #define parseurl_h_sentinel

  #define PARSEURL_PROTO_GOPHER 1
  #define PARSEURL_PROTO_HTTP 2
  #define PARSEURL_PROTO_TELNET 3
  #define PARSEURL_PROTO_UNKNOWN -1

  /* explodes a URL into parts, and return the protocol id, or a negative value on error */
  int parsegopherurl(char *url, char *host, unsigned short hostlen, unsigned short *port, char *itemtype, char *selector, unsigned short selectorlen);

  /* builds a URL from exploded parts */
  int buildgopherurl(char *res, int maxlen, int protocol, const char *host, unsigned short port, char itemtype, const char *selector);

#endif
