/*
  Copyright (c) 1999 Rafal Wojtczuk <nergal@7bulls.com>. All rights reserved.
  See the file COPYING for license details.
*/
#ifndef _NIDS_TCP_H
#define _NIDS_TCP_H
#include <sys/time.h>

#define DEBUG_REASSEMBLY printf
struct skbuff {
  struct skbuff *next;
  struct skbuff *prev;

  void *data;
  u_int len;
  u_int truesize;
  u_int urg_ptr;
  u_int seq;
  u_int ack;
  struct timeval capture_time;
  
  char fin;
  char urg;
};

int tcp_init(int);
void tcp_exit(void);
void process_tcp(u_char *, int, struct timeval *capture_time);
void process_icmp(u_char *);
void tcp_check_timeouts(struct timeval *);

#endif /* _NIDS_TCP_H */
