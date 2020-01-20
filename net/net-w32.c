/*
 * This file is part of the Gopherus project.
 * It provides a set of basic network-related functions.
 *
 * Copyright (C) Mateusz Viste 2013-2020
 *
 * Provides all network functions used by Gopherus, wrapped around the
 * Watt-32 TCP/IP stack.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/* Watt32 includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <tcp.h>

#include "net.h" /* include self for control */


#define BUFFERSIZE  4096

#define debugMode 0
#define verboseMode 0




int net_dnsresolve(char *ip, const char *name) {
  unsigned long ipnum;
  ipnum = resolve(name); /* I could use WatTCP's lookup_host() here to do all
                            the job for me, unfortunately lookup_host() issues
                            wild outs() calls putting garbage on screen... */
  if (ipnum == 0) return(-1);
  _inet_ntoa(ip, ipnum); /* convert to string */
  return(0);
}


static int dummy_printf(const char * format, ...) {
  if (format == NULL) return(-1);
  return(0);
}


/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void) {
  tzset();
  _printf = dummy_printf;  /* this is to avoid watt32 printing its stuff to console */
  return(sock_init());
}


struct net_tcpsocket *net_connect(const char *ipstr, unsigned short port) {
  struct net_tcpsocket *resultsock;
  unsigned long ipaddr;

  /* convert ip to value */
  ipaddr = _inet_addr(ipstr);
  if (ipaddr == 0) return(NULL);

  resultsock = malloc(sizeof(struct net_tcpsocket) + BUFFERSIZE);
  if (resultsock == NULL) return(NULL);
  resultsock->sock   = calloc(1, sizeof(tcp_Socket));
  if (resultsock->sock == NULL) {
    free(resultsock);
    return(NULL);
  }

  sock_setbuf(resultsock->sock, resultsock->buffer, BUFFERSIZE);
  if (!tcp_open(resultsock->sock, 0, ipaddr, port, NULL)) {
    sock_abort(resultsock->sock);
    free(resultsock->sock);
    free(resultsock);
    return(NULL);
  }

  return(resultsock);
}


int net_isconnected(struct net_tcpsocket *s, int waitstate) {
  waitstate = waitstate; /* gcc warning shut */
  if (tcp_tick(s->sock) == 0) return(-1);
  if (sock_established(s->sock) == 0) return(0);
  return(1);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and <0 otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_send(struct net_tcpsocket *socket, const char *line, long len) {
  int res;
  /* call this to let Watt-32 handle its internal stuff */
  if (tcp_tick(socket->sock) == 0) return(-1);
  /* send bytes */
  res = sock_write(socket->sock, line, len);
  return(res);
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, char *buff, long maxlen) {
  int i;
  /* call this to let WatTCP hanle its internal stuff */
  if (tcp_tick(socket->sock) == 0) return(-1);
  i = sock_fastread(socket->sock, buff, maxlen);
  return(i);
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  int status = 0;
  sock_close(socket->sock);
  sock_wait_closed(socket->sock, sock_delay, NULL, &status);
 sock_err:
  free(socket->sock);
  free(socket);
  return;
}


/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket) {
  sock_abort(socket->sock);
  free(socket->sock);
  free(socket);
  return;
}


const char *net_engine(void) {
  return(wattcpVersion());
}
