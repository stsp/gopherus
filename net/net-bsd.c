/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013-2020
 *
 * Provides all network functions used by Gopherus, wrapped around POSIX (BSD)
 * sockets (with a few messy ifdefs to support windows)
 */

#include <fcntl.h>   /* fcntl() */
#include <stdlib.h>  /* NULL */
#include <errno.h>   /* EAGAIN, EWOULDBLOCK... */
#include <stdint.h>  /* uint32_t */
#include <string.h>  /* memcpy() */

#ifdef _WIN32
  #include <winsock2.h> /* socket() */
  #include <io.h>       /* closesocket() */
  #define CLOSESOCK(x) closesocket(x)
#else
  #include <sys/socket.h> /* socket() */
  #include <sys/select.h> /* select(), fd_set() */
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h> /* close() */
  #define CLOSESOCK(x) close(x)
#endif

#include "net.h" /* include self for control */


int net_dnsresolve(char *ip, const char *name) {
  struct addrinfo *r;
  const char *ntopres;

  /* resolve */
  if (getaddrinfo(name, NULL, NULL, &r) != 0) return(-1);

  /* convert to string */
  if (r->ai_family == AF_INET) {
    struct sockaddr_in *a = (void *)(r->ai_addr);
    ntopres = inet_ntop(r->ai_family, &(a->sin_addr), ip, INET_ADDRSTRLEN);
  } else if (r->ai_family == AF_INET6) {
    struct sockaddr_in6 *a = (void *)(r->ai_addr);
    ntopres = inet_ntop(r->ai_family, a->sin6_addr.s6_addr, ip, INET6_ADDRSTRLEN);
  } else {
    ntopres = NULL; /* error */
  }

  /* free intermediary result */
  freeaddrinfo(r);

  /* return exit code */
  if (ntopres != NULL) return(0);
  return(-1);
}


/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void) {
#if _WIN32
  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  return(iResult);
#else
  return(0);
#endif
}


struct net_tcpsocket *net_connect(const char *ipaddr, unsigned short port) {
  struct sockaddr_in remote4;
  struct sockaddr_in6 remote6;
  struct net_tcpsocket *result;
  struct sockaddr_storage dstbin;
  int connectres;
  int af = AF_UNSPEC;
  int i;

  /* detect AF family */
  for (i = 0;; i++) {
    if (ipaddr[i] == '.') {
      af = AF_INET;
      break;
    }
    if (ipaddr[i] == ':') {
      af = AF_INET6;
      break;
    }
    if (ipaddr[i] == 0) return(NULL);
  }

  if (inet_pton(af, ipaddr, &dstbin) != 1) return(NULL);

  result = malloc(sizeof(struct net_tcpsocket));
  if (result == NULL) {
    return(NULL);
  }

  result->s = socket(af, SOCK_STREAM, IPPROTO_TCP);
  if (result->s < 0) {
    free(result);
    return(NULL);
  }

  /* set socket non-blocking */
  {
#if _WIN32
  unsigned long flag = 1;
  ioctlsocket(result->s, FIONBIO, &flag);
#else
    int flags;
    flags = fcntl(result->s, F_GETFD);
    fcntl(result->s, F_SETFL, flags | O_NONBLOCK);
#endif
  }

  if (af == AF_INET) {
    remote4.sin_family = AF_INET;  /* Proto family (IPv4) */
    memcpy(&(remote4.sin_addr.s_addr), &dstbin, sizeof(remote4.sin_addr.s_addr));
    remote4.sin_port = htons(port); /* set the dst port */
    connectres = connect(result->s, (struct sockaddr *)&remote4, sizeof(struct sockaddr_in));
  } else {
    remote6.sin6_family = AF_INET6;  /* Proto family (IPv6) */
    memcpy(remote6.sin6_addr.s6_addr, &dstbin, sizeof(remote6.sin6_addr.s6_addr));
    remote6.sin6_port = htons(port); /* set the dst port */
    connectres = connect(result->s, (struct sockaddr *)&remote6, sizeof(struct sockaddr_in6));
  }
#ifdef _WIN32
  if ((connectres < 0) && (WSAGetLastError() != WSAEWOULDBLOCK)) {
#else
  if ((connectres < 0) && (errno != EINPROGRESS)) {
#endif
    CLOSESOCK(result->s);
    free(result);
    return(NULL);
  }
  return(result);
}


int net_isconnected(struct net_tcpsocket *s, int waitstate) {
  fd_set set;
  struct timeval t;
  int res;
  t.tv_sec = 0;
  t.tv_usec = 0;
  if (waitstate) t.tv_usec = 8000; /* wait up to 8 ms */
  FD_ZERO(&set);
  FD_SET(s->s, &set);
  /* check socket for writeability */
  res = select(s->s + 1, NULL, &set, NULL, &t);
  if (res < 0) return(-1);
  if (res == 0) return(0);
#ifndef _WIN32
{
  socklen_t sizeofint = sizeof(int);
  /* use getsockopt(2) to read the SO_ERROR option at level SOL_SOCKET to
   * determine whether connect() completed successfully (SO_ERROR is zero) */
  getsockopt(s->s, SOL_SOCKET, SO_ERROR, &res, &sizeofint);
  if (res != 0) return(-1);
}
#endif
  return(1);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and negative value on error */
int net_send(struct net_tcpsocket *socket, const char *line, long len) {
  int res;
  res = send(socket->s, line, len, 0);
  return(res);
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, char *buff, long maxlen) {
  int res;
  fd_set rfds;
  struct timeval tv;
  /* Use select() to wait up to 20ms if nothing awaits on the socket (spares some CPU time) */
  FD_ZERO(&rfds);
  FD_SET(socket->s, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 20000;
  res = select(socket->s + 1, &rfds, NULL, NULL, &tv);
  if (res < 0) return(-1);
  if (res == 0) return(0);
  /* read the stuff now (if any) */
  res = recv(socket->s, buff, maxlen, 0);
  if (res < 0) {
#ifdef _WIN32
    if (WSAGetLastError() == WSAEWOULDBLOCK) return(0);
#else
    if (errno == EAGAIN) return(0);
    if (errno == EWOULDBLOCK) return(0);
#endif
  }
  if (res == 0) return(-1); /* the peer performed an orderly shutdown */
  return(res);
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  CLOSESOCK(socket->s);
  free(socket);
}


/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket) {
  net_close(socket);
}


const char *net_engine(void) {
  return("BSD sockets interface");
}
