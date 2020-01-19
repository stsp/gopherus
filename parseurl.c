/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2020 Mateusz Viste
 */

#include <stdio.h>    /* snprintf() */
#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */
#include "parseurl.h" /* include self for control */

int parsegopherurl(char *url, char *host, unsigned short hostlen, unsigned short *port, char *itemtype, char *selector, unsigned short selectorlen) {
  int parserstate = 0;
  int protocol = PARSEURL_PROTO_GOPHER, x;
  char *curtoken;
  /* set default values */
  *port = 70;
  *itemtype = '1';
  *selector = 0;
  /* skip the protocol part, if present */
  for (x = 0; url[x] != 0; x++) {
    if (url[x] == '/') { /* no protocol */
      protocol = PARSEURL_PROTO_GOPHER;
      break;
    }
    if (url[x] == ':') { /* found a colon. check if it's for proto declaration */
      if (url[x + 1] == '/') {
        if (url[x + 2] == '/') {
          char *protostr = url;
          url[x] = 0;
          url += x + 3;
          if (strcasecmp(protostr, "gopher") == 0) {
              protocol = PARSEURL_PROTO_GOPHER;
            } else if (strcasecmp(protostr, "http") == 0) {
              protocol = PARSEURL_PROTO_HTTP;
              *port = 80; /* default port is 80 for HTTP */
              *itemtype = 'h';
            } else if (strcasecmp(protostr, "telnet") == 0) {
              protocol = PARSEURL_PROTO_TELNET;
              *port = 23; /* default port is 23 for telnet */
              *itemtype = '8';
            } else {
              protocol = PARSEURL_PROTO_UNKNOWN;
          }
          break;
        }
      }
      protocol = PARSEURL_PROTO_GOPHER;
      break;
    }
  }
  /* start reading the url */
  curtoken = url;
  for (; parserstate < 4; url += 1) {
    switch (parserstate) {
      case 0:  /* reading host */
        if (*url == ':') { /* a port will follow */
            *host = 0;
            curtoken = url + 1;
            parserstate = 1;
          } else if (*url == '/') { /* gopher type will follow */
            *host = 0;
            parserstate = 2;
          } else if (*url == 0) { /* end of url */
            *host = 0;
            parserstate = 4;
          } else { /* still part of the host */
            *host = *url;
            hostlen--;
            if (hostlen == 0) return(-1);
            host += 1;
        }
        break;
      case 1:  /* reading port */
        if (*url == 0) { /* end of url */
            *port = atoi(curtoken);
            parserstate = 4;
          } else if (*url == '/') {
            *url = 0; /* temporary end of string */
            *port = atoi(curtoken);
            *url = '/'; /* restore the original char */
            parserstate = 2; /* gopher type follows */
        }
        break;
      case 2:  /* reading itemtype */
        if (protocol == PARSEURL_PROTO_GOPHER) { /* if non-Gopher, skip the itemtype */
          if (*url != 0) {
              *itemtype = *url;
              parserstate = 3;
              url += 1;
            } else {
              parserstate = 4;
          }
        }
        parserstate = 3; /* go right to the url part now */
        /* fall-through */
      case 3:
        if (*url == 0) {
            *selector = 0;
            parserstate = 4;
          } else {
            *selector = *url;
            selector += 1;
            selectorlen--;
            if (selectorlen == 0) return(-2);
        }
        break;
    }
  }
  return(protocol);
}


/* computes an URL string from exploded gopher parts, and returns its length. Returns -1 on error. */
int buildgopherurl(char *res, int maxlen, int protocol, const char *host, unsigned short port, char itemtype, const char *selector) {
  int x = 0;
  maxlen -= 1;
  if (protocol == PARSEURL_PROTO_HTTP) { /* http URL */
    char portstr[8] = "";
    if (port != 80) snprintf(portstr, sizeof(portstr), ":%u", port); /* include port only if != 80 */
    x = snprintf(res, maxlen, "http://%s%s/%s", host, portstr, selector);
    return(x);
  }
  /* The proto is gopher -- validate input data */
  if (maxlen < 2) return(-1);
  if (port < 1) return(-1);
  if ((host == NULL) || (res == NULL) || (selector == NULL)) return(-1);
  if (itemtype < 33) return(-1);
  /* detect special hURL links */
  if (itemtype == 'h') {
    if ((strstr(selector, "URL:") == selector) || (strstr(selector, "/URL:") == selector)) {
      if (selector[0] == '/') selector += 1;
      x = snprintf(res, maxlen, "%s", selector + 4);
      return(x);
    }
  }
  /* telnet link? */
  if (itemtype == '8') {
    x = snprintf(res, maxlen, "telnet://%s:%u", host, port);
    return(x);
  }
  /* this is a classic gopher location */
  x = snprintf(res, maxlen, "gopher://%s", host);
  /* if empty host, return only the gopher:// string */
  if (host[0] == 0) return(x);
  /* build the url string */
  if (port != 70) x += snprintf(res + x, maxlen - x, ":%u", port);
  /* if selector is empty and itemtype is 1, then stop here */
  if ((itemtype == '1') && (selector[0] == 0)) return(x);
  x += snprintf(res + x, maxlen - x, "/%c", itemtype);
  if (x == maxlen) goto maxlenreached;
  for (; *selector != 0; selector += 1) {
    if (x == maxlen) goto maxlenreached;
    if (((unsigned)*selector <= 0x1F) || ((unsigned)*selector >= 0x80)) { /* encode unsafe chars - RFC 1738: */
        if (x + 3 > maxlen) goto maxlenreached;       /* URLs are written only with the graphic printable characters of the  */
        res[x++] = '%';                               /* US-ASCII coded character set. The octets 80-FF hexadecimal are not  */
        res[x++] = '0' + (*selector >> 4);            /* used in US-ASCII, and the octets 00-1F and 7F hexadecimal represent */
        res[x++] = '0' + (*selector & 0x0F);          /* control characters; these must be encoded. */
      } else {
        res[x++] = *selector;
    }
  }
 maxlenreached:
  res[x] = 0;
  return(x);
}
