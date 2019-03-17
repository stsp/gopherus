/***************************************************************************
 * Gopherus - a console-mode gopher client                                 *
 * Copyright (C) 2013-2019 Mateusz Viste                                   *
 *                                                                         *
 * Redistribution and use in source and binary forms, with or without      *
 * modification, are permitted provided that the following conditions are  *
 * met:                                                                    *
 *                                                                         *
 * 1. Redistributions of source code must retain the above copyright       *
 *    notice, this list of conditions and the following disclaimer.        *
 *                                                                         *
 * 2. Redistributions in binary form must reproduce the above copyright    *
 *    notice, this list of conditions and the following disclaimer in the  *
 *    documentation and/or other materials provided with the distribution. *
 *                                                                         *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A         *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      *
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    *
 *                                                                         *
 ***************************************************************************/


#include <string.h>  /* strlen() */
#include <stdlib.h>  /* malloc(), getenv() */
#include <stdio.h>   /* sprintf(), fwrite()... */
#include <time.h>    /* time_t */

#include "dnscache.h"
#include "config.h"
#include "fs/fs.h"
#include "history.h"
#include "net/net.h"
#include "parseurl.h"
#include "readflin.h"
#include "ui/ui.h"
#include "wordwrap.h"
#include "startpg.h"
#include "version.h"

#define DISPLAY_ORDER_NONE 0
#define DISPLAY_ORDER_QUIT 1
#define DISPLAY_ORDER_BACK 2
#define DISPLAY_ORDER_REFR 3

#define TXT_FORMAT_RAW 0
#define TXT_FORMAT_HTM 1


struct gopherusconfig {
  int attr_textnorm;
  int attr_menucurrent;
  int attr_menutype;
  int attr_menuerr;
  int attr_menuselectable;
  int attr_statusbarinfo;
  int attr_statusbarwarn;
  int attr_urlbar;
  int attr_urlbardeco;
  char *bookmarksfile;
};

/* statusbar content, used by set_statusbar and draw_statusbar() */
static char glob_statusbar[82];


static int hex2int(char c) {
  switch (c) {
    case '0':
      return(0);
    case '1':
      return(1);
    case '2':
      return(2);
    case '3':
      return(3);
    case '4':
      return(4);
    case '5':
      return(5);
    case '6':
      return(6);
    case '7':
      return(7);
    case '8':
      return(8);
    case '9':
      return(9);
    case 'a':
    case 'A':
      return(10);
    case 'b':
    case 'B':
      return(11);
    case 'c':
    case 'C':
      return(12);
    case 'd':
    case 'D':
      return(13);
    case 'e':
    case 'E':
      return(14);
    case 'f':
    case 'F':
      return(15);
    default:
      return(-1);
  }
}


static void loadcfg(struct gopherusconfig *cfg, char **argv) {
  char *defaultcolorscheme = "177047707818141220";
  char *colorstring;
  int x;

  /* get bookmarks file location (will be useful later) */
  cfg->bookmarksfile = bookmarks_getfname(argv[0]);

  /* */
  colorstring = getenv("GOPHERUSCOLOR");
  if (colorstring != NULL) {
    if (strlen(colorstring) == 18) {
      for (x = 0; x < 18; x++) {
        if (hex2int(colorstring[x]) < 0) {
          colorstring = NULL;
          break;
        }
      }
    } else {
      colorstring = NULL;
    }
  }
  if (colorstring == NULL) colorstring = defaultcolorscheme;
  /* interpret values from the color scheme variable */
  cfg->attr_textnorm = (hex2int(colorstring[0]) << 4) | hex2int(colorstring[1]);
  cfg->attr_statusbarinfo = (hex2int(colorstring[2]) << 4) | hex2int(colorstring[3]);
  cfg->attr_statusbarwarn = (hex2int(colorstring[4]) << 4) | hex2int(colorstring[5]);
  cfg->attr_urlbar = (hex2int(colorstring[6]) << 4) | hex2int(colorstring[7]);
  cfg->attr_urlbardeco = (hex2int(colorstring[8]) << 4) | hex2int(colorstring[9]);
  cfg->attr_menutype = (hex2int(colorstring[10]) << 4) | hex2int(colorstring[11]);
  cfg->attr_menuerr = (hex2int(colorstring[12]) << 4) | hex2int(colorstring[13]);
  cfg->attr_menuselectable = (hex2int(colorstring[14]) << 4) | hex2int(colorstring[15]);
  cfg->attr_menucurrent = (hex2int(colorstring[16]) << 4) | hex2int(colorstring[17]);
}


static void set_statusbar(const char *msg) {
  int x;
  /* accept new status message only if no message set yet */
  if (glob_statusbar[0] != 0) return;
  /* */
  for (x = 0; (x < 80) && (msg[x] != 0); x++) {
    glob_statusbar[x] = msg[x];
  }
  glob_statusbar[x] = 0;
}


static unsigned short menuline_explode(char *buffer, unsigned short bufferlen, char *itemtype, char **description, char **selector, char **host, char **port) {
  char *cursor = buffer;
  int endofline = 0, column = 0;
  if (itemtype != NULL) *itemtype = *cursor;
  cursor += 1;
  if (description != NULL) *description = cursor;
  *selector = NULL;
  *host = NULL;
  *port = NULL;
  for (; cursor < (buffer + bufferlen); cursor += 1) { /* read the whole line */
    if (*cursor == '\r') continue; /* silently ignore CR chars */
    if ((*cursor == '\t') || (*cursor == '\n')) { /* delimiter */
      if (*cursor == '\n') endofline = 1;
      *cursor = 0; /* put a NULL instead to terminate previous string */
      if (endofline != 0) {
        cursor += 1;
        break;
      }
      if (column == 0) {
        *selector = cursor + 1;
      } else if (column == 1) {
        *host = cursor + 1;
      } else if (column == 2) {
        *port = cursor + 1;
      }
      if (column < 16) column += 1;
    }
  }
  return(cursor - buffer);
}


static void addbookmarkifnotexist(struct historytype *h, struct gopherusconfig *cfg) {
  FILE *fd;
  /* check if not already in bookmarks */
  fd = fopen(cfg->bookmarksfile, "rb");
  if (fd != NULL) {
    for (;;) {
      unsigned short llen;
      char lbuf[2ul * (MAXHOSTLEN + MAXSELLEN + 8ul)];
      unsigned short iport;
      char *sel, *host, *port;
      llen = readfline(lbuf, sizeof(lbuf), fd);
      if (llen == 0) break;
      /* */
      menuline_explode(lbuf, llen, NULL, NULL, &sel, &host, &port);
      /* check if same as *h */
      iport = 70;
      if (port != NULL) iport = atoi(port);
      if (iport != h->port) continue;
      if (host == NULL) continue;
      if (strcasecmp(h->host, host) != 0) continue;
      if ((sel == NULL) && (h->selector != NULL) && (h->selector[0] != 0)) continue;
      if ((sel != NULL) && (strcmp(h->selector, sel) != 0)) continue;
      /* no difference */
      set_statusbar("!This location is already bookmarked");
      fclose(fd);
      return;
    }
    fclose(fd);
  }
  /* open for appending */
  fd = fopen(cfg->bookmarksfile, "ab");
  if (fd == NULL) {
    set_statusbar("!Bookmarks file access error");
    return;
  }
  /* add to list */
  if (h->port == 70) {
    fprintf(fd, "%c%s/%c%s\t%s\t%s\t%u\n", h->itemtype, h->host, h->itemtype, h->selector, h->selector, h->host, h->port);
  } else {
    fprintf(fd, "%c%s:%u/%c%s\t%s\t%s\t%u\n", h->itemtype, h->host, h->port, h->itemtype, h->selector, h->selector, h->host, h->port);
  }
  /* */
  fclose(fd);
  set_statusbar("Bookmark saved");
}


static void delbookmark(const char *bhost, unsigned short bport, const char *bsel, struct gopherusconfig *cfg) {
  FILE *fd;
  long woff, roff;
  char lbuf[2ul * (MAXHOSTLEN + MAXSELLEN + 8ul)];
  /* open bookmarks file */
  fd = fopen(cfg->bookmarksfile, "rb+");
  if (fd == NULL) return;
  /* read lines until match found (or eof reached) */
  for (;;) {
    unsigned short iport, llen;
    char *sel, *host, *port;
    woff = ftell(fd); /* this where I will write, if match found */
    llen = readfline(lbuf, sizeof(lbuf), fd);
    if (llen == 0) {
      fclose(fd);
      return;
    }
    /* */
    iport = 70;
    menuline_explode(lbuf, llen, NULL, NULL, &sel, &host, &port);
    if (port != NULL) iport = atoi(port);
    /* */
    if (bport != iport) continue;
    if ((bhost == NULL) && (host != NULL)) continue;
    if ((bhost != NULL) && (host == NULL)) continue;
    if ((host != NULL) && (bhost != NULL) && (strcasecmp(host, bhost) != 0)) continue;
    if ((bsel == NULL) && (sel != NULL) && (sel[0] != 0)) continue;
    if ((sel == NULL) && (bsel != NULL) && (bsel[0] != 0)) continue;
    if ((sel != NULL) && (bsel != NULL) && (strcmp(sel, bsel) != 0)) continue;
    /* match found */
    break;
  }
  /* copy rest of content */
  roff = ftell(fd);
  for (;;) {
    long chunklen;
    /* read chunk */
    fseek(fd, roff, SEEK_SET);
    chunklen = fread(lbuf, 1, sizeof(lbuf), fd);
    if (chunklen == 0) break;
    roff = ftell(fd);
    /* write chunk */
    fseek(fd, woff, SEEK_SET);
    fwrite(lbuf, 1, chunklen, fd);
    woff = ftell(fd);
  }
  fclose(fd);
  /* trim file to new size */
  filetrunc(cfg->bookmarksfile, woff);
}


static void draw_urlbar(struct historytype *history, struct gopherusconfig *cfg) {
  int url_len, x;
  char urlstr[80];
  ui_putchar('[', cfg->attr_urlbardeco, 0, 0);
  url_len = buildgopherurl(urlstr, sizeof(urlstr) - 1, history->protocol, history->host, history->port, history->itemtype, history->selector);
  for (x = 0; x < 79; x++) {
    if (x < url_len) {
      ui_putchar(urlstr[x], cfg->attr_urlbar, x+1, 0);
    } else {
      ui_putchar(' ', cfg->attr_urlbar, x+1, 0);
    }
  }
  ui_putchar(']', cfg->attr_urlbardeco, 79, 0);
}


static void draw_statusbar(struct gopherusconfig *cfg) {
  int x, y, colattr;
  char *msg = glob_statusbar;
  y = ui_getrowcount() - 1;
  if (msg[0] == '!') {
    msg += 1;
    colattr = cfg->attr_statusbarwarn; /* this is an important message */
  } else {
    colattr = cfg->attr_statusbarinfo;
  }
  for (x = 0; x < 80; x++) {
    if (msg[x] == 0) break;
    ui_putchar(msg[x], colattr, x, y); /* Using putchar because otherwise the last line will scroll the screen at its end. */
  }
  for (; x < 80; x++) ui_putchar(' ', colattr, x, y);
  glob_statusbar[0] = 0; /* make room so new content can be pushed in */
  ui_refresh();
}


/* edits a string on screen. returns 0 if the string hasn't been modified, non-zero otherwise. */
static int editstring(char *url, int maxlen, int maxdisplaylen, int xx, int yy, int attr) {
  int urllen, x, presskey, cursorpos, result = 0, displayoffset;
  urllen = strlen(url);
  cursorpos = urllen;
  ui_cursor_show();
  for (;;) {
    if (urllen > maxdisplaylen - 1) {
      displayoffset = urllen - (maxdisplaylen - 1);
    } else {
      displayoffset = 0;
    }
    if (displayoffset > cursorpos - 8) {
      displayoffset = cursorpos - 8;
      if (displayoffset < 0) displayoffset = 0;
    }
    ui_locate(yy, cursorpos + xx - displayoffset);
    for (x = 0; x < maxdisplaylen; x++) {
      if ((x + displayoffset) < urllen) {
        ui_putchar(url[x + displayoffset], attr, x+xx, yy);
      } else {
        ui_putchar(' ', attr, x+xx, yy);
      }
    }
    ui_refresh();
    presskey = ui_getkey();
    if ((presskey == 0x1B) || (presskey == 0x09)) { /* ESC or TAB */
      result = 0;
      break;
    } else if (presskey == 0x147) { /* HOME */
      cursorpos = 0;
    } else if (presskey == 0x14F) { /* END */
      cursorpos = urllen;
    } else if (presskey == 0x0D) { /* ENTER */
      url[urllen] = 0; /* terminate the URL string with a NULL terminator */
      result = -1;
      break;
    } else if (presskey == 0x14B) { /* LEFT */
      if (cursorpos > 0) cursorpos -= 1;
    } else if (presskey == 0x14D) { /* RIGHT */
      if (cursorpos < urllen) cursorpos += 1;
    } else if (presskey == 0x08) { /* BACKSPACE */
      if (cursorpos > 0) {
        int y;
        urllen -= 1;
        cursorpos -= 1;
        for (y = cursorpos; y < urllen; y++) url[y] = url[y + 1];
      }
    } else if (presskey == 0x153) { /* DEL */
      if (cursorpos < urllen) {
        int y;
        for (y = cursorpos; y < urllen; y++) url[y] = url[y+1];
        urllen -= 1;
      }
    } else if (presskey == 0xFF) { /* QUIT */
      result = 0;
      break;
    } else if ((presskey > 0x1F) && (presskey < 127)) {
      if (urllen < maxlen - 1) {
        int y;
        for (y = urllen; y > cursorpos; y--) url[y] = url[y - 1];
        url[cursorpos] = presskey;
        urllen += 1;
        cursorpos += 1;
      }
    }
  }
  ui_cursor_hide();
  return(result);
}


/* returns 0 if a new URL has been entered, non-zero otherwise */
static int edit_url(struct historytype **history, struct gopherusconfig *cfg) {
  char url[MAXURLLEN];
  int urllen;
  urllen = buildgopherurl(url, sizeof(url), (*history)->protocol, (*history)->host, (*history)->port, (*history)->itemtype, (*history)->selector);
  if (urllen < 0) return(-1);
  if (editstring(url, sizeof(url), 78, 1, 0, cfg->attr_urlbar) != 0) {
    char itemtype;
    char hostaddr[MAXHOSTLEN];
    char selector[MAXSELLEN];
    unsigned short hostport;
    int protocol;
    if ((protocol = parsegopherurl(url, hostaddr, sizeof(hostaddr), &hostport, &itemtype, selector, sizeof(selector))) >= 0) {
      history_add(history, protocol, hostaddr, hostport, itemtype, selector);
      draw_urlbar(*history, cfg);
      return(0);
    }
  }
  draw_urlbar(*history, cfg); /* the url didn't changed - redraw it and forget about the whole thing */
  return(-1);
}


/* Asks for a confirmation to quit. Returns 0 if Quit aborted, non-zero otherwise. */
static int askQuitConfirmation(struct gopherusconfig *cfg) {
  int keypress;
  set_statusbar("!YOU ARE ABOUT TO QUIT. PRESS ESC TO CONFIRM, OR ANY OTHER KEY TO ABORT.");
  draw_statusbar(cfg);
  ui_refresh();
  while ((keypress = ui_getkey()) == 0x00); /* fetch the next recognized keypress */
  if ((keypress == 0x1B) || (keypress == 0xFF)) {
    return(1);
  } else {
    return(0);
  }
}


/* downloads a gopher or http resource and write it to a file or a memory
 * buffer. if *filename is not NULL, the resource will be written in the file
 * (but a valid *buffer is still required) */
static long loadfile_buff(int protocol, char *hostaddr, unsigned short hostport, char *selector, char *buffer, long buffer_max, char *filename, struct gopherusconfig *cfg, int notui) {
  unsigned long ipaddr;
  long reslength, byteread, fdlen = 0;
  int warnflag = 0;
  char statusmsg[128];
  time_t lastrefresh = 0;
  FILE *fd = NULL;
  int headersdone = 0; /* used notably for HTTP, to localize the end of headers */
  struct net_tcpsocket *sock;
  time_t lastactivity, curtime;
  if (hostaddr[0] == '#') { /* embedded start page */
    reslength = loadembeddedstartpage(buffer, buffer_max, hostaddr + 1, cfg->bookmarksfile);
    /* open file, if downloading to a file */
    if (filename != NULL) {
      fd = fopen(filename, "rb"); /* try to open for read - this should fail */
      if (fd != NULL) {
        set_statusbar("!File already exists! Operation aborted.");
        fclose(fd);
        return(-1);
      }
      fd = fopen(filename, "wb"); /* now open for write - this will create the file */
      if (fd == NULL) { /* this should not fail */
        set_statusbar("!Error: could not create the file on disk!");
        return(-1);
      }
      fwrite(buffer, 1, reslength, fd);
      fclose(fd);
    }
    return(reslength);
  }
  ipaddr = dnscache_ask(hostaddr);
  if (ipaddr == 0) {
    sprintf(statusmsg, "Resolving '%s'...", hostaddr);
    if (notui == 0) {
      set_statusbar(statusmsg);
      draw_statusbar(cfg);
    } else {
      ui_puts(statusmsg);
    }
    ipaddr = net_dnsresolve(hostaddr);
    if (ipaddr == 0) {
      set_statusbar("!DNS resolution failed!");
      return(-1);
    }
    dnscache_add(hostaddr, ipaddr);
  }
  sprintf(statusmsg, "Connecting to %lu.%lu.%lu.%lu...", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, (ipaddr & 0xFF));
  if (notui == 0) {
    set_statusbar(statusmsg);
    draw_statusbar(cfg);
  } else {
    ui_puts(statusmsg);
  }

  sock = net_connect(ipaddr, hostport);
  if (sock == NULL) {
    set_statusbar("!Connection error!");
    return(-1);
  }
  /* wait for net_connect() to actually connect */
  for (;;) {
    int connstate;
    connstate = net_isconnected(sock, 1);
    if (connstate > 0) break;
    if (connstate < 0) {
      net_abort(sock);
      set_statusbar("!Connection error!");
      return(-1);
    }
    if (ui_kbhit()) {
      net_abort(sock);
      set_statusbar("Connection aborted by user");
      ui_getkey(); /* consume the pressed key */
      return(-1);
    }
  }
  /* */
  if (protocol == PARSEURL_PROTO_HTTP) { /* http */
    sprintf(buffer, "GET /%s HTTP/1.0\r\nHOST: %s\r\nUSER-AGENT: Gopherus\r\n\r\n", selector, hostaddr);
  } else { /* gopher */
    sprintf(buffer, "%s\r\n", selector);
  }
  if (net_send(sock, buffer, strlen(buffer)) != (int)strlen(buffer)) {
    set_statusbar("!send() error!");
    net_close(sock);
    return(-1);
  }
  /* prepare timers */
  lastactivity = time(NULL);
  curtime = lastactivity;
  /* open file, if downloading to a file */
  if (filename != NULL) {
    fd = fopen(filename, "rb"); /* try to open for read - this should fail */
    if (fd != NULL) {
      set_statusbar("!File already exists! Operation aborted.");
      fclose(fd);
      net_abort(sock);
      return(-1);
    }
    fd = fopen(filename, "wb"); /* now open for write - this will create the file */
    if (fd == NULL) { /* this should not fail */
      set_statusbar("!Error: could not create the file on disk!");
      net_abort(sock);
      return(-1);
    }
  }
  /* receive answer */
  reslength = 0;
  for (;;) {
    if (buffer_max + fdlen - reslength < 1) { /* too much data! */
      set_statusbar("!Error: Server's answer is too long! (truncated)");
      warnflag = 1;
      break;
    }
    byteread = net_recv(sock, buffer + (reslength - fdlen), buffer_max + fdlen - reslength);
    curtime = time(NULL);
    if (byteread < 0) break; /* end of connection */
    if (ui_kbhit() != 0) { /* a key has been pressed - read it */
      int presskey = ui_getkey();
      if ((presskey == 0x1B) || (presskey == 0x08)) { /* if it's escape or backspace, abort the connection */
        set_statusbar("Connection aborted by the user.");
        reslength = -1;
        break;
      }
    }
    if (byteread > 0) {
        lastactivity = curtime;
        reslength += byteread;
        /* if protocol is http, ignore headers */
        if ((protocol == PARSEURL_PROTO_HTTP) && (headersdone == 0)) {
          int i;
          for (i = 0; i < reslength - 2; i++) {
            if (buffer[i] == '\n') {
              if (buffer[i + 1] == '\r') i++; /* skip CR if following */
              if (buffer[i + 1] == '\n') {
                i += 2;
                headersdone = reslength;
                for (reslength = 0; i < headersdone; i++) buffer[reslength++] = buffer[i];
                break;
              }
            }
          }
        } else {
          /* refresh the status bar once every second */
          if (curtime != lastrefresh) {
            lastrefresh = curtime;
            sprintf(statusmsg, "Downloading... [%ld bytes]", reslength);
            if (notui == 0) {
              set_statusbar(statusmsg);
              draw_statusbar(cfg);
            } else {
              ui_puts(statusmsg);
            }
          }
          /* if downloading to file, write stuff to disk */
          if ((fd != NULL) && (reslength - fdlen > (buffer_max / 2))) {
            int writeres = fwrite(buffer, 1, reslength - fdlen, fd);
            if (writeres < 0) writeres = 0;
            fdlen += writeres;
          }
      }
    } else {
      if (curtime - lastactivity > 20) { /* TIMEOUT! */
        set_statusbar("!Timeout while waiting for data!");
        reslength = -1;
        break;
      }
    }
  }
  if ((reslength >= 0) && (warnflag == 0)) {
    if (notui == 0) set_statusbar("");
    net_close(sock);
  } else {
    net_abort(sock);
  }
  if (fd != NULL) { /* finish the buffer */
    if (reslength - fdlen > 0) { /* if anything left in the buffer, write it now */
      fdlen += fwrite(buffer, 1, reslength - fdlen, fd);
    }
    fclose(fd);
    sprintf(statusmsg, "Saved %ld bytes on disk", fdlen);
    set_statusbar(statusmsg);
  }
  return(reslength);
}


/* compute a filename proposition based on url - this is used to suggest a
 * filename when user downloads something from the gopherspace */
static void genfnamefromselector(char *fname, unsigned short maxlen, const char *selector) {
  unsigned short i, lastdot = 0xffffu, flen, extlen = 0;
  if (maxlen < 1) return;
  *fname = 0; /* worst case scenario - nothing is suggested */
  maxlen -= 1; /* make room for the null terminator */
  /* find where the last filename may start, as well as locate last dot and compute length */
  for (i = 0; selector[i] != 0; i++) {
    if (selector[i] == '/') {
      selector += i + 1;
      i = 0;
      flen = 0;
      lastdot = 0xffff;
    } else if (selector[i] == '.') {
      lastdot = i;
    }
  }
  /* compute lengths */
  if (lastdot != 0xffffu) {
    flen = lastdot;
    extlen = (i - lastdot) - 1;
    if (extlen > 0) extlen++;  /* count the dot separateor as part of ext */
  } else {
    flen = i;
  }
#ifdef NOLFN
  /* adjust flen & extlen to 8+3 */
  if (flen > 8) flen = 8;
  if (extlen > 4) extlen = 4;
#endif
  /* adjust length to maxlen */
  if (flen + extlen > maxlen) {
    flen = maxlen - extlen;
  }
  if (flen < 1) return;
  /* fill fname */
  memcpy(fname, selector, flen);
  if (extlen > 0) memcpy(fname + flen, selector + lastdot, extlen);
  fname[flen + extlen] = 0;
  /* replace shady chars by underscores */
  for (i = 0; fname[i] != 0; i++) {
    if ((fname[i] >= 'a') && (fname[i] <= 'z')) continue;
    if ((fname[i] >= 'A') && (fname[i] <= 'Z')) continue;
    if ((fname[i] >= '0') && (fname[i] <= '9')) continue;
    if ((fname[i] == '.') && (i == flen)) continue; /* dot is okay, but only before ext */
    switch (fname[i]) {
      case '_':
      case '-':
      case '@':
      case '$':
      case '(':
      case ')':
      case '.':
      case '!':
      case '&':
        continue;
    }
    /* anything else gets to be replaced */
    fname[i] = '_';
  }
}


/* used by display_menu to tell whether an itemtype is selectable or not */
static int isitemtypeselectable(char itemtype) {
  switch (itemtype) {
    case 'i':  /* inline message */
    case '3':  /* error */
    case 0:    /* special internal type for menu lines continuations */
      return(0);
    default: /* everything else is selectable */
      return(1);
  }
}


/* explodes a gopher menu into separate lines. returns amount of lines */
static long menu_explode(char *buffer, long bufferlen, char *line_itemtype, char **line_description, unsigned char *line_description_len, char **line_selector, char **line_host, unsigned short *line_port, long maxlines, long *firstlinkline, long *lastlinkline) {
  char *description, *cursor, *selector, *host, *port, itemtype;
  char singlelinebuf[82];
  long linecount = 0;

  *firstlinkline = -1;
  *lastlinkline = -1;

  for (cursor = buffer; cursor < (buffer + bufferlen) ;) {

    cursor += menuline_explode(cursor, bufferlen - (buffer - cursor), &itemtype, &description, &selector, &host, &port);

    if (itemtype == '.') continue; /* ignore lines starting by '.' - it's most probably the end of menu terminator */
    if (linecount < maxlines) {
      char *wrapptr = description;
      int wraplen;
      int firstiteration = 0;
      if (isitemtypeselectable(itemtype) != 0) {
        if (*firstlinkline < 0) *firstlinkline = linecount;
        *lastlinkline = linecount;
      }
      for (;; firstiteration += 1) {
        if ((firstiteration > 0) && (itemtype != 'i') && (itemtype != '3')) itemtype = 0;
        if (itemtype == 'i') {
          wraplen = 80;
        } else {
          wraplen = 76;
        }
        line_description[linecount] = wrapptr;
        wrapptr = wordwrap(wrapptr, singlelinebuf, wraplen);
        line_description_len[linecount] = strlen(singlelinebuf);
        line_selector[linecount] = selector;
        line_host[linecount] = host;
        line_itemtype[linecount] = itemtype;
        if (port != NULL) {
          line_port[linecount] = atoi(port);
          if (line_port[linecount] < 1) line_port[linecount] = 70;
        } else {
          line_port[linecount] = 70;
        }
        linecount += 1;
        if (wrapptr == NULL) break;
      }
    } else {
      set_statusbar("!ERROR: Too many lines, the document has been truncated.");
      break;
    }
  }

  /* trim out the last line if its starting with a '.' (gopher's end of menu marker) */
  if (linecount > 0) {
    if (line_itemtype[linecount - 1] == '.') linecount -= 1;
  }
  return(linecount);
}


static int display_menu(struct historytype **history, struct gopherusconfig *cfg, char *buffer, long buffersize) {
  long bufferlen, linecount;
  char *line_description[MAXMENULINES];
  char *line_selector[MAXMENULINES];
  char *line_host[MAXMENULINES];
  char curURL[MAXURLLEN];
  unsigned short line_port[MAXMENULINES];
  char line_itemtype[MAXMENULINES];
  unsigned char line_description_len[MAXMENULINES];
  long x, y;
  long *selectedline = &(*history)->displaymemory[0];
  long *screenlineoffset = &(*history)->displaymemory[1];
  long firstlinkline, lastlinkline;
  int keypress;
  if (*screenlineoffset < 0) *screenlineoffset = 0;

  /* copy the history content into buffer - we need to do this because we'll perform changes on the data */
  bufferlen = (*history)->cachesize;
  if (bufferlen > buffersize) bufferlen = buffersize;
  memcpy(buffer, (*history)->cache, bufferlen);
  buffer[bufferlen] = 0;
  /* */
  linecount = menu_explode(buffer, bufferlen, line_itemtype, line_description, line_description_len, line_selector, line_host, line_port, MAXMENULINES, &firstlinkline, &lastlinkline);

  /* if there is at least one position, and nothing is selected yet, make it active */
  if ((firstlinkline >= 0) && (*selectedline < 0)) *selectedline = firstlinkline;

  for (;;) {
    curURL[0] = 0;
    /* if any position is selected, print the url in status bar */
    if (*selectedline >= 0) {
      buildgopherurl(curURL, sizeof(curURL), PARSEURL_PROTO_GOPHER, line_host[*selectedline], line_port[*selectedline], line_itemtype[*selectedline], line_selector[*selectedline]);
      if (glob_statusbar[0] == 0) set_statusbar(curURL);
    }
    /* start drawing lines of the menu */
    for (x = *screenlineoffset; x < *screenlineoffset + (ui_getrowcount() - 2); x++) {
      if (x < linecount) {
        int z, attr;
        char *prefix = NULL;
        attr = cfg->attr_menuselectable;
        if (x == *selectedline) { /* change the background if item is selected */
          attr = cfg->attr_menucurrent;
        } else {
          attr = cfg->attr_menutype;
        }
        switch (line_itemtype[x]) {
          case 'i': /* message */
            break;
          case 'h': /* html */
            prefix = "HTM";
            break;
          case '0': /* text */
            prefix = "TXT";
            break;
          case '1':
            prefix = "DIR";
            break;
          case '3':
            prefix = "ERR";
            break;
          case '5':
          case '9':
            prefix = "BIN";
            break;
          case '7':
            prefix = "ASK";
            break;
          case 'I':
          case 'g': /* GIF */
            prefix = "IMG";
            break;
          case 'P':
          case 'd':
            prefix = "PDF";
            break;
          case 0: /* this is an internal itemtype that means 'it's a continuation of the previous (wrapped) line */
            prefix = "   ";
            break;
          default: /* unknown type */
            prefix = "UNK";
            break;
        }
        z = 0;
        if (prefix != NULL) {
          for (y = 0; y < 3; y++) ui_putchar(prefix[y], attr, y, 1 + (x - *screenlineoffset));
          ui_putchar(' ', attr, y, 1 + (x - *screenlineoffset));
          z = 4;
        }
        /* select foreground color */
        if (x == *selectedline) {
          attr = cfg->attr_menucurrent;
        } else if (line_itemtype[x] == 'i') {
          attr = cfg->attr_textnorm;
        } else if (line_itemtype[x] == '3') {
          attr = cfg->attr_menuerr;
        } else {
          if (isitemtypeselectable(line_itemtype[x]) != 0) {
            attr = cfg->attr_menuselectable;
          } else {
            attr = cfg->attr_textnorm;
          }
        }
        /* print the the line's description */
        for (y = 0; y < (80 - z); y++) {
          if (y < line_description_len[x]) {
            ui_putchar(line_description[x][y], attr, y + z, 1 + (x - *screenlineoffset));
          } else {
            ui_putchar(' ', attr, y + z, 1 + (x - *screenlineoffset));
          }
        }
      } else { /* x >= linecount */
        for (y = 0; y < 80; y++) ui_putchar(' ', cfg->attr_textnorm, y, 1 + (x - *screenlineoffset));
      }
    }
    draw_statusbar(cfg);
    ui_refresh();
    /* wait for a keypress */
    keypress = ui_getkey();
    switch (keypress) {
      case 0x08: /* BACKSPACE */
        return(DISPLAY_ORDER_BACK);
        break;
      case 0x09: /* TAB */
        if (edit_url(history, cfg) == 0) return(DISPLAY_ORDER_NONE);
        break;
      case 'b':
        addbookmarkifnotexist(*history, cfg);
        break;
      case 0x143: /* F9 */
      case 0x0D: /* ENTER */
        if (*selectedline >= 0) {
          if ((line_itemtype[*selectedline] == '7') && (keypress != 0x143)) { /* a query needs to be issued */
            char query[MAXQUERYLEN];
            char *finalselector;
            set_statusbar("Enter a query: ");
            draw_statusbar(cfg);
            query[0] = 0;
            if (editstring(query, sizeof(query), 64, 15, ui_getrowcount() - 1, cfg->attr_statusbarinfo) == 0) break;
            finalselector = malloc(strlen(line_selector[*selectedline]) + strlen(query) + 2); /* add 1 for the TAB, and 1 for the NULL terminator */
            if (finalselector == NULL) {
              set_statusbar("Out of memory");
              break;
            } else {
              sprintf(finalselector, "%s\t%s", line_selector[*selectedline], query);
              history_add(history, PARSEURL_PROTO_GOPHER, line_host[*selectedline], line_port[*selectedline], line_itemtype[*selectedline], finalselector);
              free(finalselector);
              return(DISPLAY_ORDER_NONE);
            }
          } else { /* itemtype is anything else than type 7 */
            int tmpproto;
            unsigned short tmpport;
            char tmphost[MAXHOSTLEN], tmpitemtype, tmpselector[MAXSELLEN];
            tmpproto = parsegopherurl(curURL, tmphost, sizeof(tmphost), &tmpport, &tmpitemtype, tmpselector, sizeof(tmpselector));
            if (keypress == 0x143) tmpitemtype = '9'; /* force the itemtype to 'binary' if 'save as' was requested */
            if (tmpproto < 0) {
              set_statusbar("!Bad URL");
              break;
            } else {
              history_add(history, tmpproto, tmphost, tmpport, tmpitemtype, tmpselector);
              return(DISPLAY_ORDER_NONE);
            }
          }
        }
        break;
      case 0x144: /* F10 - download all items from current directory */
        if (firstlinkline >= 0) {
          for (x = firstlinkline; x <= lastlinkline; x++) {
            char fname[32];
            char b[512];
            if (isitemtypeselectable(line_itemtype[x]) == 0) continue;
            /* generate a filename for the target */
            genfnamefromselector(fname, sizeof(fname), line_selector[x]);
            /* TODO watch out for already-existing files! */
            /* download the file */
            loadfile_buff(PARSEURL_PROTO_GOPHER, line_host[x], line_port[x], line_selector[x], b, sizeof(b), fname, cfg, 0);
          }
        }
        break;
      case 0x153: /* DEL */
        if ((history[0]->host[0] == '#') && (history[0]->host[1] == 'w')) {
          delbookmark(line_host[*selectedline], line_port[*selectedline], line_selector[*selectedline], cfg);
          return(DISPLAY_ORDER_REFR);
        }
        break;
      case 0x1B: /* Esc */
        if (askQuitConfirmation(cfg) != 0) return(DISPLAY_ORDER_QUIT);
        break;
      case 0x13B: /* F1 - help */
        history_add(history, PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
        return(DISPLAY_ORDER_NONE);
        break;
      case 0x13C: /* F2 - home */
        history_add(history, PARSEURL_PROTO_GOPHER, "#welcome", 70, '1', "");
        return(DISPLAY_ORDER_NONE);
        break;
      case 0x13F: /* F5 - refresh */
        return(DISPLAY_ORDER_REFR);
        break;
      case 0x147: /* HOME */
        if (*selectedline >= 0) *selectedline = firstlinkline;
        *screenlineoffset = 0;
        break;
      case 0x148: /* UP */
        if (*selectedline > firstlinkline) {
          long prevlink = *selectedline;
          /* find the next item that is selectable */
          while (isitemtypeselectable(line_itemtype[--prevlink]) == 0);
          /* if prevlink is on screen, select it */
          if (prevlink >= *screenlineoffset) {
            *selectedline = prevlink;
          } else { /* move screen up, if possible... */
            if (*screenlineoffset > 0) *screenlineoffset -= 1;
            /* ...and recheck */
            if (prevlink >= *screenlineoffset) *selectedline = prevlink;
          }
        } else {
          if (*screenlineoffset > 0) *screenlineoffset -= 1;
          continue; /* do not force the selected line to be on screen */
        }
        break;
      case 0x149: /* PGUP */
        if (*screenlineoffset >= (ui_getrowcount() - 2)) {
          *screenlineoffset -= ui_getrowcount() - 2;
        } else {
          *screenlineoffset = 0;
        }
        /* select last visible link (if any, and unless currently selected
         * menu link is somehow still visible) */
        if (*selectedline >= (*screenlineoffset + ui_getrowcount() - 2)) {
          long i;
          for (i = *screenlineoffset; i < (*screenlineoffset + ui_getrowcount() - 2); i++) {
            if (i >= linecount) break;
            if (isitemtypeselectable(line_itemtype[i]) != 0) {
              *selectedline = i;
            }
          }
        }
        break;
      case 0x14F: /* END */
        if (*selectedline >= 0) *selectedline = lastlinkline;
        *screenlineoffset = linecount - (ui_getrowcount() - 3);
        if (*screenlineoffset < 0) *screenlineoffset = 0;
        break;
      case 0x150: /* DOWN */
        if (*selectedline > *screenlineoffset + ui_getrowcount() - 3) { /* if selected line is below the screen, don't change the selection */
          *screenlineoffset += 1;
          continue;
        }
        /* select next link, if possible */
        if (*selectedline < lastlinkline) {
          long nextlink = *selectedline;
          /* find the next selectable item */
          while (isitemtypeselectable(line_itemtype[++nextlink]) == 0);
          /* if next link is within screen area, select it */
          if ((nextlink >= *screenlineoffset) && (nextlink <= (*screenlineoffset + (ui_getrowcount() - 3)))) {
            *selectedline = nextlink;
          } else {
            /* move screen down */
            if (*screenlineoffset < linecount - (ui_getrowcount() - 3)) {
              *screenlineoffset += 1;
            }
            /* if next link is within screen area, select it */
            if ((nextlink >= *screenlineoffset) && (nextlink <= (*screenlineoffset + (ui_getrowcount() - 3)))) {
              *selectedline = nextlink;
            }
          }
        } else {
          /* just move screen down, if not at last line already */
          if (*screenlineoffset < linecount - (ui_getrowcount() - 3)) {
            *screenlineoffset += 1;
          }
          continue; /* do not force the selected line to be on screen */
        }
        break;
      case 0x151: /* PGDOWN */
        if (*screenlineoffset < linecount - (ui_getrowcount() - 2)) {
          *screenlineoffset += ui_getrowcount() - 2;
        }
        /* select first visible link (if any, and unless currently selected
         * menu link is somehow still visible) */
        if (*selectedline < *screenlineoffset) {
          long i;
          for (i = *screenlineoffset; i < (*screenlineoffset + ui_getrowcount() - 2); i++) {
            if (i >= linecount) break;
            if (isitemtypeselectable(line_itemtype[i]) != 0) {
              *selectedline = i;
              break;
            }
          }
        }
        break;
      case 0xFF:  /* 0xFF -> quit immediately */
        return(1);
        break;
      default:
      /* {
        char sbuf[16];
        sprintf(sbuf, "KEY 0x%02X", keypress);
        set_statusbar(sbuf);
      } */
        break;
    }
  }
}


static int display_text(struct historytype **history, struct gopherusconfig *cfg, char *buffer, long buffersize, int txtformat) {
  char *txtptr;
  char linebuff[128];
  long x, y, firstline, lastline, bufferlen;
  int eof_flag;

  sprintf(linebuff, "file loaded (%ld bytes)", (*history)->cachesize);
  set_statusbar(linebuff);

  /* copy the content of the file into buffer, and take care to modify dangerous chars and apply formating (if any) */
  bufferlen = 0;
  if (txtformat == TXT_FORMAT_HTM) { /* HTML format */
    int lastcharwasaspace = 0;
    int insidetoken = -1;
    int insidescript = 0;
    int insidebody = 0;
    char token[8];
    char specialchar[8];
    int insidespecialchar = -1;
    for (x = 0; x < (*history)->cachesize; x++) {
      if ((bufferlen + 4) > buffersize) break;
      if ((insidescript != 0) && (insidetoken < 0) && ((*history)->cache[x] != '<')) continue;
      switch ((*history)->cache[x]) {
        case '\t':  /* replace whitespaces by single spaces */
        case '\n':
        case '\r':
        case ' ':
          if (insidetoken >= 0) {
            if (insidetoken < 7) token[insidetoken++] = 0;
            continue;
          }
          if (lastcharwasaspace == 0) {
            buffer[bufferlen++] = ' ';
            lastcharwasaspace = 1;
          }
          break;
        case '<':
          lastcharwasaspace = 0;
          insidetoken = 0;
          break;
        case '>':
          lastcharwasaspace = 0;
          if (insidetoken < 0) continue;
          token[insidetoken] = 0;
          insidetoken = -1;
          if ((strcasecmp(token, "/p") == 0) || (strcasecmp(token, "br") == 0) || (strcasecmp(token, "/tr") == 0) || (strcasecmp(token, "/title") == 0)) {
            buffer[bufferlen++] = '\n';
          } else if (strcasecmp(token, "script") == 0) {
            insidescript = 1;
          } else if (strcasecmp(token, "body") == 0) {
            insidebody = 1;
          } else if (strcasecmp(token, "/script") == 0) {
            insidescript = 0;
          }
          break;
        default:
          lastcharwasaspace = 0;
          if (insidetoken >= 0) {
            if (insidetoken < 7) token[insidetoken++] = (*history)->cache[x];
            continue;
          }
          if ((insidespecialchar < 0) && ((*history)->cache[x] == '&')) {
            insidespecialchar = 0;
            continue;
          }
          if ((insidespecialchar >= 0) && (insidespecialchar < 7)) {
            if ((*history)->cache[x] != ';') {
              specialchar[insidespecialchar++] = (*history)->cache[x];
              continue;
            }
            specialchar[insidespecialchar] = 0;
            if (strcasecmp(specialchar, "nbsp") == 0) {
              buffer[bufferlen++] = ' ';
            } else {
              buffer[bufferlen++] = '_';
            }
            insidespecialchar = -1;
            continue;
          }
          if ((*history)->cache[x] < 32) break; /* ignore ascii control chars */
          if (insidebody == 0) break; /* ignore everything until <body> starts */
          buffer[bufferlen++] = (*history)->cache[x]; /* copy everything else */
          break;
      }
    }
  } else { /* process content as raw text */
    for (x = 0; x < (*history)->cachesize; x++) {
      if (bufferlen + 10 > buffersize) break;
      switch ((*history)->cache[x]) {
        case 8:     /* replace tabs by 8 spaces */
          for (y = 0; y < 8; y++) buffer[bufferlen++] = ' ';
          break;
        case '\n':  /* preserve line feeds */
          buffer[bufferlen++] = '\n';
          break;
        case '\r':  /* ignore CR chars */
        case 127:   /* as well as DEL chars */
          break;
        default:
          if ((*history)->cache[x] < 32) break; /* ignore ascii control chars */
          buffer[bufferlen++] = (*history)->cache[x]; /* copy everything else */
          break;
      }
    }
    /* check if there is a single . on the last line */
    if ((buffer[bufferlen - 1] == '\n') && (buffer[bufferlen - 2] == '.')) bufferlen -= 2;
  }
  /* terminate the buffer with a NULL terminator */
  buffer[bufferlen] = 0;
  /* display the file on screen */
  firstline = 0;
  lastline = ui_getrowcount() - 3;
  for (;;) { /* display-control loop */
    y = 0;
    for (txtptr = buffer; txtptr != NULL; ) {
      txtptr = wordwrap(txtptr, linebuff, 80);
      if (y >= firstline) {
        int endstringreached = 0;
        for (x = 0; x < 80; x++) {
          if (linebuff[x] == 0) endstringreached = 1;
          if (endstringreached == 0) {
            ui_putchar(linebuff[x], cfg->attr_textnorm, x, y + 1 - firstline);
          } else {
            ui_putchar(' ', cfg->attr_textnorm, x, y + 1 - firstline);
          }
        }
      }
      y++;
      if (y > lastline) break;
    }
    if (y <= lastline) {
      eof_flag = 1;
      for (; y <= lastline ; y++) { /* fill the rest of the screen (if any left) with blanks */
        for (x = 0; x < 80; x++) ui_putchar(' ', cfg->attr_textnorm, x, y + 1 - firstline);
      }
    } else {
      eof_flag = 0;
    }
    draw_statusbar(cfg);
    ui_refresh();
    x = ui_getkey();
    switch (x) {
      case 0x08:   /* Backspace */
        return(DISPLAY_ORDER_BACK);
        break;
      case 0x09: /* TAB */
        if (edit_url(history, cfg) == 0) return(DISPLAY_ORDER_NONE);
        break;
      case 'b':
        addbookmarkifnotexist(*history, cfg);
        break;
      case 0x1B:   /* ESC */
        if (askQuitConfirmation(cfg) != 0) return(DISPLAY_ORDER_QUIT);
        break;
      case 0x13B: /* F1 - help */
        history_add(history, PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
        return(DISPLAY_ORDER_NONE);
        break;
      case 0x13C: /* F2 - home */
        history_add(history, PARSEURL_PROTO_GOPHER, "#welcome", 70, '1', "");
        return(DISPLAY_ORDER_NONE);
        break;
      case 0x13F: /* F5 - refresh */
        return(DISPLAY_ORDER_REFR);
        break;
      case 0x143: /* F9 - download */
        history_add(history, (*history)->protocol, (*history)->host, (*history)->port, '9', (*history)->selector);
        return(DISPLAY_ORDER_NONE);
        break;
      case 0x148: /* UP */
        if (firstline > 0) {
          firstline -= 1;
          lastline -= 1;
        } else {
          set_statusbar("Reached the top of the file");
        }
        break;
      case 0x150: /* DOWN */
        if (eof_flag == 0) {
          firstline += 1;
          lastline += 1;
        } else {
          set_statusbar("Reached end of file");
        }
        break;
      case 0x147: /* HOME */
        lastline -= firstline;
        firstline = 0;
        break;
      case 0x149: /* PGUP */
        if (firstline > 0) {
          firstline -= ui_getrowcount() - 3;
          if (firstline < 0) firstline = 0;
          lastline = firstline + ui_getrowcount() - 3;
        } else {
          set_statusbar("Reached the top of the file");
        }
        break;
      case 0x14F: /* END */
        break;
      case 0x151: /* PGDOWN */
        if (eof_flag == 0) {
          firstline += ui_getrowcount() - 3;
          lastline += ui_getrowcount() - 3;
        } else {
          set_statusbar("Reached end of file");
        }
        break;
      case 0xFF: /* QUIT IMMEDIATELY */
        return(1);
        break;
      default:  /* unhandled key */
        /* sprintf(linebuff, "Got invalid key: 0x%02lX", x);
        set_statusbar(linebuff); */
        break;
    }
  }
}



int main(int argc, char **argv) {
  int exitflag;
  char *fatalerr = NULL;
  char *buffer;
  char *saveas = NULL;
  int bufferlen;
  struct historytype *history = NULL;
  struct gopherusconfig cfg;

  /* Load configuration (or defaults) */
  loadcfg(&cfg, argv);

  /* alloc page buffer */
  buffer = malloc(PAGEBUFSZ);

  if ((buffer == NULL) || (history_add(&history, PARSEURL_PROTO_GOPHER, "#welcome", 70, '1', "") != 0)) {
    free(buffer);
    ui_puts("Out of memory.");
    return(2);
  }

  if (argc > 1) { /* if some params have been received, parse them */
    char itemtype;
    char hostaddr[MAXHOSTLEN];
    char selector[MAXSELLEN];
    unsigned short hostport, i;
    int protocol;
    int goturl = 0;
    for (i = 1; i < argc; i++) {
      /* recognize valid options */
      if ((argv[i][0] == '-') && (argv[i][1] == 'o') && (argv[i][2] == '=') && (saveas == NULL)) {
        saveas = argv[i] + 3;
        continue;
      } else if ((argv[i][0] == '/') || (argv[i][0] == '-')) { /* unknown parameter */
        ui_puts("Gopherus v" pVer " Copyright (C) " pDate " Mateusz Viste");
        ui_puts("");
        ui_puts("Usage: gopherus [url [-o=outfile]]");
        ui_puts("");
        free(buffer);
        return(1);
      }
      /* assume it is an url then */
      if (goturl != 0) {
        ui_puts("Invalid parameters list.");
        free(buffer);
        return(1);
      }
      if ((protocol = parsegopherurl(argv[i], hostaddr, sizeof(hostaddr), &hostport, &itemtype, selector, sizeof(selector))) < 0) {
        ui_puts("Invalid URL!");
        free(buffer);
        return(1);
      }
      goturl = 1;
      history_add(&history, protocol, hostaddr, hostport, itemtype, selector);
      if (history == NULL) {
        ui_puts("Out of memory.");
        free(buffer);
        return(2);
      }
    }
  }

  if (net_init() != 0) {
    ui_puts("Network subsystem initialization failed!");
    free(buffer);
    return(3);
  }

  /* if in non-interactive mode (-o=...), then fetch the resource and quit */
  if (saveas != NULL) {
    long res;
    if ((history == NULL) || (history->host[0] == '#')) {
      ui_puts("You must provide an URL when using -o=...");
      res = -1;
    } else {
      res = loadfile_buff(history->protocol, history->host, history->port, history->selector, buffer, PAGEBUFSZ, saveas, &cfg, 1);
    }
    /* unallocate all the history */
    history_flush(history);
    /* */
    if (res < 1) {
      ui_puts("Error: failed to fetch the remote resource");
      free(buffer);
      return(1);
    }
    /* return to the OS */
    free(buffer);
    return(0);
  }

  ui_init();
  ui_cursor_hide(); /* hide the cursor */
  ui_cls();

  for (;;) {
    if ((history->itemtype == '0') || (history->itemtype == '1') || (history->itemtype == '7') || (history->itemtype == 'h')) { /* if it's a displayable item type... */
      draw_urlbar(history, &cfg);
      if (history->cache == NULL) { /* reload the resource if not in cache already */
        bufferlen = loadfile_buff(history->protocol, history->host, history->port, history->selector, buffer, PAGEBUFSZ, NULL, &cfg, 0);
        if (bufferlen < 0) {
          history_back(&history);
          continue;
        } else {
          history_cleanupcache(history);
          if (bufferlen == 0) { /* malloc(0) may return NULL on some platforms */
            history->cache = malloc(1);
          } else {
            history->cache = malloc(bufferlen);
          }
          if (history->cache == NULL) {
            history_back(&history);
            set_statusbar("!Out of memory!");
            continue;
          }
          if (bufferlen > 0) memcpy(history->cache, buffer, bufferlen);
          history->cachesize = bufferlen;
        }
      }
      switch (history->itemtype) {
        case '0': /* text file */
          exitflag = display_text(&history, &cfg, buffer, PAGEBUFSZ, TXT_FORMAT_RAW);
          break;
        case 'h': /* html file */
          exitflag = display_text(&history, &cfg, buffer, PAGEBUFSZ, TXT_FORMAT_HTM);
          break;
        case '1': /* menu */
        case '7': /* query result (also a menu) */
          exitflag = display_menu(&history, &cfg, buffer, PAGEBUFSZ);
          break;
        default:
          fatalerr = "Fatal error: got an unhandled itemtype!";
          exitflag = DISPLAY_ORDER_QUIT;
          break;
      }
      if (exitflag == DISPLAY_ORDER_BACK) {
        history_back(&history);
      } else if (exitflag == DISPLAY_ORDER_REFR) {
        free(history->cache);
        history->cache = NULL;
        history->cachesize = 0;
        history->displaymemory[0] = -1;
        history->displaymemory[1] = -1;
      } else if (exitflag == DISPLAY_ORDER_QUIT) {
        break;
      }
    } else { /* the itemtype is not one of the internally displayable types -> ask to download it */
      char filename[64];
      const char *prompt = "Download as: ";
      int i;
      genfnamefromselector(filename, sizeof(filename), history->selector);
      set_statusbar("");
      draw_statusbar(&cfg);
      for (i = 0; prompt[i] != 0; i++) ui_putchar(prompt[i], 0x70, i, ui_getrowcount() - 1);
      if (editstring(filename, 63, 63, i, ui_getrowcount() - 1, 0x70) != 0) {
        loadfile_buff(history->protocol, history->host, history->port, history->selector, buffer, PAGEBUFSZ, filename, &cfg, 0);
      }
      history_back(&history);
    }
  }
  ui_cursor_show(); /* unhide the cursor */
  ui_cls();
  if (fatalerr != NULL) ui_puts(fatalerr); /* display error message, if any */
  /* unallocate all the history */
  history_flush(history);

  ui_close();
  free(buffer);

  return(0);
}
