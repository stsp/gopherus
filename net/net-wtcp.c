/*
 *  This file is part of the Gopherus project.
 *  It provides a set of basic network-related functions.
 *
 *  Copyright (C) Mateusz Viste 2013-2018
 *
 * Provides all network functions used by Gopherus, wrapped around the WatTCP
 * TCP/IP stack.
 *
 * THIS CODE IS NOT FUNCTIONAL - PROTOTYPE ONLY!
 */

#include <stdlib.h>

/* WatTCP includes */
#include <wattcp.h>

#include "net.h" /* include self for control */


/* this is a wrapper around the wattcp resolve().
 * returns 0 if resolutin fails. */
unsigned long net_dnsresolve(const char *name) {
  unsigned long hostaddr;
  hostaddr = resolve((char *)name);
  return(hostaddr);
}


/*static int dummy_printf(const char * format, ...) {
  if (format == NULL) return(-1);
  return(0);
}*/

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void) {
  sock_init();
  return(0);
}


struct net_tcpsocket *net_connect(unsigned long ipaddr, int port) {
  return(NULL);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and <0 otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_send(struct net_tcpsocket *socket, char *line, int len) {
  return(-1);
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen) {
  return(-1);
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  return;
}


/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket) {
  return;
}


/* Translates a libtcp result from its single integer code into a humanly readable message string.
Returns a pointer to the translated string. */
char *net_strerror(int socket_result) {
  if (socket_result == 0) return("");
  return("");
}
