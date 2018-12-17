/*
 *  This file is part of the Gopherus project.
 *  It provides a set of basic network-related functions.
 *
 *  Copyright (C) Mateusz Viste 2013-2018
 *
 * Provides all network functions used by Gopherus, wrapped around the
 * picotcp stack.
 */

#include <stdlib.h>
#include <time.h>
#include <i86.h>

#include <pico_stack.h>
#include <pico_dns_client.h>
#include <picodos.h>

#include "net.h" /* include self for control */


#define debugMode 0
#define verboseMode 0


static struct pico_device *picodev;
static volatile int dnswait = 0;
static volatile char *dnsres = NULL;

/* callback for pico_dns_client_getaddr() */
static void cb_dns(char *addr, void *arg) {
  dnsres = addr;
  dnswait = 0;
}

/* this is a wrapper around the wattcp lookup_host(), but with a small integrated cache.
   returns 0 if resolution fails. */
unsigned long net_dnsresolve(const char *name) {
  unsigned long res;
  int crudetimer;
  time_t t;
  pico_dns_client_getaddr(name, cb_dns, NULL);
  /* wait for the callback to provide resolution, or time out */
  t = time(NULL);
  crudetimer = 0;
  dnswait = 1;
  while (dnswait) {
    union REGS regs;
    int86(0x28, &regs, &regs); /* DOS 2+ IDLE INTERRUPT */
    pico_stack_tick();
    if (time(NULL) != t) {
      t = time(NULL);
      crudetimer++;
      if (crudetimer > 3) return(0);
    }
  }
  /* convert string to long, free dnsres and return to caller */
  pico_string_to_ipv4((char *)dnsres, &res);
  free((char *)dnsres);
  return(res);
}

static void freepico(void) {
  picoquit(picodev);
  free(picodev);
}

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void) {
  picodev = malloc(sizeof(struct pico_device));
  if (picodev == NULL) return(-1);
  if (picoinit(picodev, 0) != 0) {
    free(picodev);
    return(-2);
  }
  atexit(freepico);
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
  return("");
}
