/***************************************************************************
 * Gopherus - a console-mode gopher client                                 *
 * Copyright (C) 2013-2022 Mateusz Viste                                   *
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
#include <stdint.h>
#include <stdlib.h>  /* malloc(), getenv() */
#include <stdio.h>   /* snprintf(), fwrite()... */
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
static char glob_statusbar[128];


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
  /* accept new status message only if no message set yet */
  if (glob_statusbar[0] != 0) return;
  /* copy msg to statusbar, watch out for overflows */
  snprintf(glob_statusbar, sizeof(glob_statusbar), "%s", msg);
}


/* parses a buffer that contains a gopher menu and processes the first menu entry
 * fills itemtype with the gopher type of the entry and sets description, selector, host and port pointers so they point to the corresponding value in the menu.
 * returns length of the parsed line */
static unsigned short menuline_explode(char *buffer, unsigned short bufferlen, char *itemtype, char **description, char **selector, char **host, char **port) {
  char *cursor = buffer;
  int column = 0;
  if (itemtype != NULL) *itemtype = *cursor;
  cursor += 1;
  if (description != NULL) *description = cursor;
  *selector = NULL;
  *host = NULL;
  *port = NULL;
  for (; cursor < (buffer + bufferlen); cursor++) { /* read the whole line */
    /* silently ignore CR chars */
    if (*cursor == '\r') continue;

    /* end of line, I'm done */
    if (*cursor == '\n') {
      *cursor = 0; /* put a NULL instead to terminate previous string */
      cursor += 1;
      break;
    }

    /* delimiter */
    if (*cursor == '\t') { /* delimiter */
      *cursor = 0; /* put a NULL instead to terminate previous string */
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


/* used by drawstr to decode utf8 strings */
static uint32_t utf8toint(unsigned char s) {
  static uint32_t buff = 0;
  static int waitfor;
  /* single-byte value? */
  if ((s & 0x80) == 0) {
    if (waitfor != 0) goto DECODE_ERR; /* decoding error */
    buff = 0;
    return(s);
  }

  /* handle multi-byte sequences */
  if ((s & 0xC0) == 0x80) { /* continuation? */
    if (waitfor == 0) goto DECODE_ERR; /* decoding error */
    waitfor--;
    buff <<= 6;
    buff |= (s & 0x3F);
    if (waitfor == 0) return(buff);
  } else if ((s & 0xE0) == 0xC0) { /* 2-byte value? (110xxxxx) */
    if (waitfor != 0) goto DECODE_ERR;
    waitfor = 1;
    buff = s & 0x1F;
  } else if ((s & 0xF0) == 0xE0) { /* 3-byte value? (1110xxxx) */
    if (waitfor != 0) goto DECODE_ERR;
    waitfor = 2;
    buff = s & 0x0F;
  } else if ((s & 0xF8) == 0xF0) { /* 4-byte value? (1111xxxx) */
    if (waitfor != 0) goto DECODE_ERR;
    waitfor = 3;
    buff = s & 0x07;
  } else { /* unhandled seq */
    goto DECODE_ERR;
  }
  return(0);

  DECODE_ERR:
  waitfor = 0;
  buff = 0;
  return('.');
}


/* print string on screen and space-fill it to len characters if needed
 * NOTE: this shall be the only function used by gopherus to write on screen!
 * s may be an UTF-8 string (may or may not be rendered properly depending on
 * the ui target) */
static void drawstr(const char *s, int attr, int x, int y, int len) {
  int i, l;
  uint32_t wchar;

  l = 0;
  for (i = 0; (s[i] != 0) && (l < len); i++) {
    wchar = utf8toint(s[i]);
    if (wchar == 0) continue;

    /* replace TABs by spaces */
    if (wchar == '\t') wchar = ' ';

    /* don't try printing ascii representation of a control char */
    if (wchar < 32) wchar = '.';
    ui_putchar(wchar, attr, x + l, y);
    l++;
  }

  for (; l < len; l++) ui_putchar(' ', attr, x + l, y);
  ui_refresh();
}


static void addbookmarkifnotexist(const struct historytype *h, const struct gopherusconfig *cfg) {
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
    if ((h->selector == NULL) || (h->selector[0] == 0)) {
      fprintf(fd, "%c%s\t\t%s\t%u\n", h->itemtype, h->host, h->host, h->port);
    } else {
      fprintf(fd, "%c%s/%c%s\t%s\t%s\t%u\n", h->itemtype, h->host, h->itemtype, h->selector, h->selector, h->host, h->port);
    }
  } else {
    if ((h->selector == NULL) || (h->selector[0] == 0)) {
      fprintf(fd, "%c%s:%u\t\t%s\t%u\n", h->itemtype, h->host, h->port, h->host, h->port);
    } else {
      fprintf(fd, "%c%s:%u/%c%s\t%s\t%s\t%u\n", h->itemtype, h->host, h->port, h->itemtype, h->selector, h->selector, h->host, h->port);
    }
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
  char urlstr[256];
  ui_putchar('[', cfg->attr_urlbardeco, 0, 0);
  buildgopherurl(urlstr, sizeof(urlstr) - 1, history->protocol, history->host, history->port, history->itemtype, history->selector);
  drawstr(urlstr, cfg->attr_urlbar, 1, 0, ui_getcolcount() - 2);
  ui_putchar(']', cfg->attr_urlbardeco, ui_getcolcount() - 1, 0);
  ui_refresh();
}


static void draw_statusbar(struct gopherusconfig *cfg) {
  int y, colattr;
  char *msg = glob_statusbar;
  y = ui_getrowcount() - 1;
  if (msg[0] == '!') {
    msg += 1;
    colattr = cfg->attr_statusbarwarn; /* this is an important message */
  } else {
    colattr = cfg->attr_statusbarinfo;
  }
  drawstr(msg, colattr, 0, y, ui_getcolcount());
  glob_statusbar[0] = 0; /* make room so new content can be pushed in */
}


/* edits a string on screen. returns 0 if the string hasn't been modified, non-zero otherwise. */
static int editstring(char *url, int maxlen, int maxdisplaylen, int xx, int yy, int attr) {
  int urllen, presskey, cursorpos, result = 0, displayoffset;
  urllen = strlen(url);
  cursorpos = urllen;
  ui_cursor_show();
  for (;;) {
    url[urllen] = 0; /* always make sure the URL is NULL-terminated */
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
    drawstr(url + displayoffset, attr, xx, yy, maxdisplaylen);
    presskey = ui_getkey();
    if ((presskey == 0x1B) || (presskey == 0x09)) { /* ESC or TAB */
      result = 0;
      break;
    } else if (presskey == 0x147) { /* HOME */
      cursorpos = 0;
    } else if (presskey == 0x14F) { /* END */
      cursorpos = urllen;
    } else if (presskey == 0x0D) { /* ENTER */
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
  if (editstring(url, sizeof(url), ui_getcolcount() - 2, 1, 0, cfg->attr_urlbar) != 0) {
    char itemtype;
    char hostaddr[MAXHOSTLEN];
    char selector[MAXSELLEN];
    unsigned short hostport;
    unsigned char protocol;
    if ((protocol = parsegopherurl(url, hostaddr, sizeof(hostaddr), &hostport, &itemtype, selector, sizeof(selector))) != PARSEURL_ERROR) {
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


static long http_skip_headers(char *buffer, long buffersz, struct net_tcpsocket *sock, unsigned short timeout) {
  long res = 0;
  long i;
  int bytesread;
  time_t now = time(NULL);

  FETCHNEXTCHUNK:

  /* abort on timeout */
  if (time(NULL) - now > timeout) return(-1);

  /* abort if buffer full - this should never happen, HTTP headers are not that huge */
  if (res >= buffersz) return(-1);

  bytesread = net_recv(sock, buffer + res, buffersz - res);

  /* reloop if got nothing */
  if (bytesread == 0) goto FETCHNEXTCHUNK;

  /* error (end of connection) */
  if (bytesread < 0) return(-1);

  /* got some data, let's look */
  res += bytesread;

  for (i = 0; i < res - 2; i++) {

    /* find next LF */
    if (buffer[i] != '\n') continue;

    /* skip the trailing CR, if present */
    if (buffer[i + 1] == '\r') i++;

    /* is the next byte an LF? then I reached the end of HTTP headers */
    if (buffer[i + 1] == '\n') {
      i += 2;
      /* move any leftover data to the front of the buffer and quit */
      if (res - i > 0) memmove(buffer, buffer + i, res - i);
      return(res - i);
    }
  }
  goto FETCHNEXTCHUNK;
}


/* downloads a gopher or http resource and write it to a file or a memory
 * buffer. if *filename is not NULL, the resource will be written in the file
 * (but a valid *buffer is still required) */
static long loadfile_buff(unsigned char protocol, char *hostaddr, unsigned short hostport, char *selector, char *buffer, long buffer_max, char *filename, struct gopherusconfig *cfg, int notui) {
  char ipaddr[64];
  long reslength, byteread, totlen = 0;
  int warnflag = 0;
  char statusmsg[128];
  time_t lastrefresh = 0;
  FILE *fd = NULL;
  struct net_tcpsocket *sock = NULL;
  time_t lastactivity, curtime;

  /* is the call for an embedded page? */
  if (hostaddr[0] == '#') {
    reslength = loadembeddedstartpage(buffer, buffer_max, hostaddr + 1, cfg->bookmarksfile);
    /* open file, if downloading to a file */
    if (filename != NULL) {
      fd = fopen(filename, "rb"); /* try to open for read - this should fail */
      if (fd != NULL) {
        set_statusbar("!File already exists! Operation aborted.");
        goto FAIL;
      }
      fd = fopen(filename, "wb"); /* now open for write - this will create the file */
      if (fd == NULL) { /* this should not fail */
        set_statusbar("!Error: could not create the file on disk!");
        goto FAIL;
      }
      fwrite(buffer, 1, reslength, fd);
      fclose(fd);
    }
    return(reslength);
  }

  /* DNS resolution */
  if (dnscache_ask(ipaddr, hostaddr) != 0) {
    snprintf(statusmsg, sizeof(statusmsg), "Resolving '%s'...", hostaddr);
    if (notui == 0) {
      set_statusbar(statusmsg);
      draw_statusbar(cfg);
    } else {
      ui_puts(statusmsg);
    }
    if (net_dnsresolve(ipaddr, hostaddr) != 0) {
      set_statusbar("!DNS resolution failed!");
      goto FAIL;
    }
    dnscache_add(hostaddr, ipaddr);
  }

  /* connect */
  snprintf(statusmsg, sizeof(statusmsg), "Connecting to %s...", ipaddr);
  if (notui == 0) {
    set_statusbar(statusmsg);
    draw_statusbar(cfg);
  } else {
    ui_puts(statusmsg);
  }

  sock = net_connect(ipaddr, hostport);
  if (sock == NULL) {
    set_statusbar("!Connection error!");
    goto FAIL;
  }

  /* wait for net_connect() to actually connect */
  for (;;) {
    int connstate;
    connstate = net_isconnected(sock, 1);
    if (connstate > 0) break;
    if (connstate < 0) {
      set_statusbar("!Connection error!");
      goto FAIL;
    }
    if (ui_kbhit()) {
      set_statusbar("Connection aborted by user");
      ui_getkey(); /* consume the pressed key */
      goto FAIL;
    }
  }

  /* build and send the query */
  if (protocol == PARSEURL_PROTO_HTTP) { /* http */
    snprintf(buffer, buffer_max, "GET /%s HTTP/1.0\r\nHOST: %s\r\nUSER-AGENT: Gopherus\r\n\r\n", selector, hostaddr);
  } else { /* gopher */
    snprintf(buffer, buffer_max, "%s\r\n", selector);
  }
  if (net_send(sock, buffer, strlen(buffer)) != (int)strlen(buffer)) {
    set_statusbar("!send() error!");
    goto FAIL;
  }

  /* prepare timers */
  lastactivity = time(NULL);
  curtime = lastactivity;

  /* open file, if downloading to a file */
  if (filename != NULL) {
    fd = fopen(filename, "rb"); /* try to open for read - this should fail */
    if (fd != NULL) {
      set_statusbar("!File already exists! Operation aborted.");
      goto FAIL;
    }
    fd = fopen(filename, "wb"); /* now open for write - this will create the file */
    if (fd == NULL) { /* this should not fail */
      set_statusbar("!Error: could not create the file on disk!");
      goto FAIL;
    }
  }

  /* zero out reslength */
  reslength = 0;

  /* if protocol is http, ignore headers */
  if (protocol == PARSEURL_PROTO_HTTP) {
    reslength = http_skip_headers(buffer, buffer_max, sock, 2);
    if (reslength < 0) {
      set_statusbar("!Error: Failed to fetch or parse HTTP headers");
      goto FAIL;
    }
  }

  /* receive payload */
  for (;;) {

    /* a key has been pressed - read it */
    if (ui_kbhit() != 0) {
      int presskey = ui_getkey();
      if ((presskey == 0x1B) || (presskey == 0x08)) { /* if it's escape or backspace, abort the connection */
        set_statusbar("Connection aborted by the user.");
        goto FAIL;
      }
    }

    /* look out for buffer space */
    if (reslength >= buffer_max) { /* too much data! */
      snprintf(statusmsg, sizeof(statusmsg), "!Error: Server's answer is too long! (truncated to %ld bytes)", reslength);
      set_statusbar(statusmsg);
      warnflag = 1;
      break;
    }

    byteread = net_recv(sock, buffer + reslength, buffer_max - reslength);
    curtime = time(NULL);

    /* got nothing */
    if (byteread == 0) {
      if (curtime - lastactivity > 20) { /* TIMEOUT! */
        set_statusbar("!Timeout while waiting for data!");
        reslength = -1;
        goto FAIL;
      }
      continue;
    }

    /* end of connection */
    if (byteread < 0) break;

    /* got something */
    lastactivity = curtime;
    totlen += byteread;

    /* if downloading to file, write data to disk */
    if (fd != NULL) {
      if ((long)fwrite(buffer, 1, byteread, fd) != byteread) {
        set_statusbar("!Error while writing data to disk");
        draw_statusbar(cfg);
        goto FAIL;
      }
    } else { /* else just keep it in the buffer */
      reslength += byteread;
    }

    /* refresh the status bar once every second */
    if (curtime != lastrefresh) {
      lastrefresh = curtime;
      snprintf(statusmsg, sizeof(statusmsg), "Downloading... [%ld bytes]", totlen);
      if (notui == 0) {
        set_statusbar(statusmsg);
        draw_statusbar(cfg);
      } else {
        ui_puts(statusmsg);
      }
    }
  }

  if ((reslength >= 0) && (warnflag == 0)) {
    if (notui == 0) set_statusbar("");
    net_close(sock);
  } else {
    net_abort(sock);
  }

  if (fd != NULL) { /* if downloading to file: print message */
    snprintf(statusmsg, sizeof(statusmsg), "Saved %ld bytes on disk", totlen);
    set_statusbar(statusmsg);
  }

  return(reslength);

  FAIL:
  if (fd != NULL) fclose(fd);
  if (sock != NULL) net_abort(sock);
  return(-1);
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
  if (itemtype & 128) return(0); /* line continuations are not selectable */
  switch (itemtype) {
    case 'i':  /* inline message */
    case '3':  /* error */
    case '.':  /* end of menu marker */
      return(0);
    default: /* everything else is selectable */
      return(1);
  }
}


static int isitemtypedownloadable(char itemtype) {
  if (isitemtypeselectable(itemtype) == 0) return(0); /* if you can't select it, then you can't download it */
  if (itemtype == '8') return(0); /* telnet links cannot be downloaded */
  /* anything else should be ok */
  return(1);
}


/* explodes a gopher menu into separate lines. returns amount of lines */
static long menu_explode(char *buffer, long bufferlen, unsigned char *line_itemtype, char **line_description, unsigned short *line_selector, unsigned short *line_host, unsigned short *line_port, long maxlines, long *firstlinkline, long *lastlinkline) {
  char *cursor;
  char *singlelinebuf;
  long linecount = 0;
  int screenw = ui_getcolcount();

  singlelinebuf = malloc(screenw + 2);

  *firstlinkline = -1;
  *lastlinkline = -1;

  for (cursor = buffer; bufferlen > 0;) {
    unsigned char colid = 0;
    char *selector = NULL;
    char *host = NULL;
    char *port = NULL;
    char *lineorigin;
    char itemtype;

    /* abort if too many lines already */
    if (linecount >= maxlines) {
      set_statusbar("!ERROR: Too many lines, the document has been truncated.");
      break;
    }

    /* remember where the line starts */
    lineorigin = cursor;

    /* advance cursor to next line, change all tabs to nuls on the way and remember the position of the 3 first columns (selector, host, port) */
    while (bufferlen) {
      if (*cursor == '\t') {
        *cursor = 0;
        switch (colid) {
          case 0: /* selector */
            selector = cursor + 1;
            break;
          case 1: /* host */
            host = cursor + 1;
            break;
          case 2: /* port */
            port = cursor + 1;
            break;
        }
        colid++;
      }
      cursor++;
      bufferlen--;
      if (*cursor == '\r') *cursor = 0;
      if (cursor[-1] == '\n') {
        cursor[-1] = 0;
        break;
      }
    }

    /* consider empty lines as informational (i) */
    if (lineorigin[0] != 0) {
      itemtype = *lineorigin;
    } else {
      itemtype = 'i';
    }

    { /* line-wrapping business */
      char *wrapptr = lineorigin + 1;
      int wraplen;
      int firstiteration = 1;
      if (isitemtypeselectable(itemtype) != 0) {
        if (*firstlinkline < 0) *firstlinkline = linecount;
        *lastlinkline = linecount;
      }
      for (;; firstiteration = 0) {
        if (itemtype == 'i') {
          wraplen = screenw;
        } else {
          wraplen = screenw - 4;
        }
        line_description[linecount] = wrapptr;
        wrapptr = wordwrap(wrapptr, singlelinebuf, wraplen);
        line_description[linecount][strlen(singlelinebuf)] = 0; /* terminate line after wrap */
        if (selector == NULL) {
          line_selector[linecount] = 0;
        } else {
          line_selector[linecount] = (selector - line_description[linecount]);
        }
        if (host == NULL) {
          line_host[linecount] = 0;
        } else {
          line_host[linecount] = (host - line_description[linecount]);
        }
        if (port == NULL) {
          line_port[linecount] = 70;
        } else {
          line_port[linecount] = atol(port);
          if (line_port[linecount] == 0) line_port[linecount] = 70;
        }
        line_itemtype[linecount] = itemtype;
        if (!firstiteration) line_itemtype[linecount] |= 128;
        linecount++;
        if (wrapptr == NULL) break;
      }
    }
  }

  /* trim out the last line if its starting with a '.' (gopher's "end of menu" marker) */
  if (linecount > 0) {
    if (line_itemtype[linecount - 1] == '.') linecount--;
  }

  /* trim out all trailing empty lines */
  while ((linecount > 0) && (line_description[linecount - 1][0] == 0)) linecount--;

  free(singlelinebuf);

  return(linecount);
}


static int display_menu(struct historytype **history, struct gopherusconfig *cfg, char *buffer, long buffersize) {
  long bufferlen, linecount;
  char *line_description[MAXMENULINES];
  unsigned short line_selector_off[MAXMENULINES];
  unsigned short line_host_off[MAXMENULINES];
  unsigned short line_port[MAXMENULINES];
  unsigned char line_itemtype[MAXMENULINES];
  char curURL[MAXURLLEN];
  long x;
  long *selectedline = &(*history)->displaymemory[0];
  long *screenlineoffset = &(*history)->displaymemory[1];
  long firstlinkline, lastlinkline;
  int keypress;

  if (*screenlineoffset < 0) *screenlineoffset = 0;

  /* copy the history content into buffer - we need to do this because we'll perform changes on the data */
  bufferlen = (*history)->cachesize;
  if (bufferlen >= buffersize) bufferlen = buffersize - 1; /* -1 for the final nul terminator */
  memcpy(buffer, (*history)->cache, bufferlen);
  buffer[bufferlen] = 0;
  /* */
  linecount = menu_explode(buffer, bufferlen, line_itemtype, line_description, line_selector_off, line_host_off, line_port, MAXMENULINES, &firstlinkline, &lastlinkline);

  /* if there is at least one position, and nothing is selected yet, make it active */
  if ((firstlinkline >= 0) && (*selectedline < 0)) *selectedline = firstlinkline;

  for (;;) {
    curURL[0] = 0;

    /* if any position is selected, fetch the selected values and print the url in status bar */
    if (*selectedline >= 0) {
      buildgopherurl(curURL, sizeof(curURL), PARSEURL_PROTO_GOPHER, line_description[*selectedline] + line_host_off[*selectedline], line_port[*selectedline], line_itemtype[*selectedline] & 127, line_description[*selectedline] + line_selector_off[*selectedline]);
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
        switch (line_itemtype[x] & 127) {
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
          case '8':
            prefix = "TLN";
            break;
          case 'I':
          case 'g': /* GIF */
            prefix = "IMG";
            break;
          case 'P':
          case 'd':
            prefix = "PDF";
            break;
          default: /* unknown type */
            prefix = "UNK";
            break;
        }
        if ((line_itemtype[x] & 128) && (prefix != NULL)) prefix = "   ";
        z = 0;
        if (prefix != NULL) {
          drawstr(prefix, attr, 0, 1 + (x - *screenlineoffset), 4);
          z = 4;
        }
        /* select foreground color */
        if (x == *selectedline) {
          attr = cfg->attr_menucurrent;
        } else if ((line_itemtype[x] & 127) == 'i') {
          attr = cfg->attr_textnorm;
        } else if ((line_itemtype[x] & 127) == '3') {
          attr = cfg->attr_menuerr;
        } else {
          if (isitemtypeselectable(line_itemtype[x] & 127) != 0) {
            attr = cfg->attr_menuselectable;
          } else {
            attr = cfg->attr_textnorm;
          }
        }
        /* print the the line's description */
        drawstr(line_description[x], attr, 0 + z, 1 + (x - *screenlineoffset), ui_getcolcount() - z);
      } else { /* x >= linecount */
        drawstr("", cfg->attr_textnorm, 0, 1 + (x - *screenlineoffset), ui_getcolcount());
      }
    }
    draw_urlbar(*history, cfg);
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
        if (*selectedline < 0) break; /* no effect if no menu entry is selected */
        if (((line_itemtype[*selectedline] & 127) == '7') && (keypress != 0x143)) { /* a query needs to be issued */
          char query[MAXQUERYLEN];
          char *finalselector;
          size_t finalselectorsz;
          set_statusbar("Enter a query: ");
          draw_statusbar(cfg);
          query[0] = 0;
          if (editstring(query, sizeof(query), 64, 15, ui_getrowcount() - 1, cfg->attr_statusbarinfo) == 0) break;
          finalselectorsz = strlen(line_description[*selectedline] + line_selector_off[*selectedline]) + strlen(query) + 2; /* add 1 for the TAB, and 1 for the NULL terminator */
          finalselector = malloc(finalselectorsz);
          if (finalselector == NULL) {
            set_statusbar("!Out of memory");
            break;
          }
          snprintf(finalselector, finalselectorsz, "%s\t%s", line_description[*selectedline] + line_selector_off[*selectedline], query);
          history_add(history, PARSEURL_PROTO_GOPHER, line_description[*selectedline] + line_host_off[*selectedline], line_port[*selectedline], line_itemtype[*selectedline] & 127, finalselector);
          free(finalselector);
          return(DISPLAY_ORDER_NONE);
        } else { /* itemtype is anything else than type 7 */
          unsigned char tmpproto;
          unsigned short tmpport;
          char tmphost[MAXHOSTLEN], tmpitemtype, tmpselector[MAXSELLEN];
          tmpproto = parsegopherurl(curURL, tmphost, sizeof(tmphost), &tmpport, &tmpitemtype, tmpselector, sizeof(tmpselector));
          if (keypress == 0x143) { /* if 'save as' was requested, force itemtype unless it's not downloadable */
            if (isitemtypedownloadable(tmpitemtype) == 0) {
              set_statusbar("!This item type cannot be downloaded");
              break;
            }
            tmpitemtype = '9'; /* force the itemtype to 'binary' if 'save as' was requested */
          }
          if (tmpproto == PARSEURL_ERROR) {
            set_statusbar("!Bad URL");
            break;
          } else if ((tmpproto == PARSEURL_PROTO_GOPHER) || (tmpproto == PARSEURL_PROTO_HTTP)) {
            history_add(history, tmpproto, tmphost, tmpport, tmpitemtype, tmpselector);
            return(DISPLAY_ORDER_NONE);
          } else {
            set_statusbar("!Unsupported protocol");
            break;
          }
        }
        break;
      case 0x144: /* F10 - download all items from current directory */
        if (firstlinkline >= 0) {
          for (x = firstlinkline; x <= lastlinkline; x++) {
            char fname[32];
            char b[512];
            /* skip not downloadable items */
            if (isitemtypedownloadable(line_itemtype[x]) == 0) continue;
            /* generate a filename for the target */
            genfnamefromselector(fname, sizeof(fname), line_description[x] + line_selector_off[x]);
            /* TODO watch out for already-existing files! */
            /* download the file */
            loadfile_buff(PARSEURL_PROTO_GOPHER, line_description[x] + line_host_off[x], line_port[x], line_description[x] + line_selector_off[x], b, sizeof(b), fname, cfg, 0);
          }
        }
        break;
      case 0x153: /* DEL */
        if ((history[0]->host[0] == '#') && (history[0]->host[1] == 'w')) {
          delbookmark(line_description[*selectedline] + line_host_off[*selectedline], line_port[*selectedline], line_description[*selectedline] + line_selector_off[*selectedline], cfg);
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
  char linebuff[256];
  long x, y, firstline, lastline, bufferlen;
  int eof_flag;
  int screenw = ui_getcolcount();

  if (screenw > (int)sizeof(linebuff) - 1) screenw = (int)sizeof(linebuff) - 1;

  snprintf(linebuff, sizeof(linebuff), "file loaded (%ld bytes)", (*history)->cachesize);
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
          if (((*history)->cache[x] >= 0) && ((*history)->cache[x] < 32)) break; /* ignore ascii control chars */
          buffer[bufferlen++] = (*history)->cache[x]; /* copy everything else */
          break;
      }
    }
    /* check if there is a single . on the last line */
    if ((buffer[bufferlen - 1] == '\n') && (buffer[bufferlen - 2] == '.')) bufferlen -= 2;
  }
  /* terminate the buffer with a nul terminator */
  buffer[bufferlen] = 0;
  /* display the file on screen */
  firstline = 0;
  lastline = ui_getrowcount() - 3;
  for (;;) { /* display-control loop */
    y = 0;
    for (txtptr = buffer; txtptr != NULL; ) {
      txtptr = wordwrap(txtptr, linebuff, screenw);
      if (y >= firstline) {
        drawstr(linebuff, cfg->attr_textnorm, 0, y + 1 - firstline, ui_getcolcount());
      }
      y++;
      if (y > lastline) break;
    }
    if (y <= lastline) {
      eof_flag = 1;
      for (; y <= lastline ; y++) { /* fill the rest of the screen (if any left) with blanks */
        drawstr("", cfg->attr_textnorm, 0, y + 1 - firstline, ui_getcolcount());
      }
    } else {
      eof_flag = 0;
    }
    draw_urlbar(*history, cfg);
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


/* creates a bookmark file with some default entries if no bookmark file exists yet */
static void bookmarkfile_createifnone(const char *fname) {
  FILE *fd;
  if (fname == NULL) return;

  /* abort if bookmarks file exists already */
  fd = fopen(fname, "rb");
  if (fd != NULL) {
    fclose(fd);
    return;
  }

  /* create new file */
  fd = fopen(fname, "wb");
  if (fd == NULL) return; /* oops */

  /* populate some default links */
  fprintf(fd, "1gopher.viste.fr (Gopherus author's gopher home)\t\tgopher.viste.fr\n"
              "1gopherpedia.com\t\tgopherpedia.com\n"
              "1gopher.floodgap.com\t\tgopher.floodgap.com\n");

  /* close file */
  fclose(fd);
}


int main(int argc, char **argv) {
  int exitflag;
  char *fatalerr = NULL;
  char *buffer;
  char *saveas = NULL;
  struct historytype *history = NULL;
  struct gopherusconfig cfg;

  /* Load configuration (or defaults) */
  loadcfg(&cfg, argv);

  /* alloc page buffer + 1 byte for a guardian value to detect overflows */
  buffer = malloc(PAGEBUFSZ + 1);
  if ((buffer == NULL) || (history_add(&history, PARSEURL_PROTO_GOPHER, "#welcome", 70, '1', "") != 0)) {
    free(buffer);
    ui_puts("Out of memory.");
    return(2);
  }
  buffer[PAGEBUFSZ] = '#';

  if (argc > 1) { /* if some params have been received, parse them */
    char itemtype;
    char hostaddr[MAXHOSTLEN];
    char selector[MAXSELLEN];
    unsigned short hostport, i;
    unsigned char protocol;
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
        ui_puts("Latest version can be found at the following addresses:");
        ui_puts("  http://gopherus.sourceforge.net");
        ui_puts("  gopher://gopher.viste.fr");
        ui_puts("");
        ui_puts("This build is dated \"" __DATE__ "\" and handles networking through:");
        ui_puts(net_engine());
        free(buffer);
        return(1);
      }
      /* assume it is an url then */
      if (goturl != 0) {
        ui_puts("Invalid parameters list.");
        free(buffer);
        return(1);
      }
      if ((protocol = parsegopherurl(argv[i], hostaddr, sizeof(hostaddr), &hostport, &itemtype, selector, sizeof(selector))) == PARSEURL_ERROR) {
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

  if (ui_init() != 0) {
    ui_puts("ERROR: ui_init() failure");
    free(buffer);
    return(4);
  }

  ui_cursor_hide(); /* hide the cursor */
  ui_cls();

  /* create a bookmark file template if it does not exist yet */
  bookmarkfile_createifnone(cfg.bookmarksfile);

  for (;;) {
    if ((history->itemtype == '0') || (history->itemtype == '1') || (history->itemtype == '7') || (history->itemtype == 'h')) { /* if it's a displayable item type... */
      draw_urlbar(history, &cfg);
      if (history->cache == NULL) { /* reload the resource if not in cache already */
        long bufferlen;
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
      int i;
      const char *prompt = "Download as: ";
      genfnamefromselector(filename, sizeof(filename), history->selector);
      set_statusbar("");
      draw_statusbar(&cfg);
      i = strlen(prompt);
      drawstr(prompt, 0x70, 0, ui_getrowcount() - 1, i);
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

  /* look for buffer overflow */
  if (buffer[PAGEBUFSZ] != '#') ui_puts("WARNING: BUFFER OVERFLOW DETECTED");
  free(buffer);

  return(0);
}
