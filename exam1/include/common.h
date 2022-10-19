/*
 * These are functions used by multiple modules and contains odds and ends.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <arpa/inet.h>          /* htons */
#include <ifaddrs.h>            /* getifaddrs, sockaddr_ll */
#include <linux/if_packet.h>    /* AF_PACKET, struct sockaddr_ll */
#include <stdint.h>             /* uint8_t */
#include <stdio.h>              /* printf */
#include <stdlib.h>             /* free */
#include <string.h>             /* memset */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>         /* socket */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <errno.h>              /* errno */
#include <unistd.h>             /* close */

#include "ether.h"
#include "mip.h"
#include "mip_arp.h"

#define SOCKET_NAME "server.socket"
#define MAX_CONNS 5             /* max. length of the pending connections queue 
                                 */

/* This macro is dangerous! It relies on the variable `msg` being called
 * exactly that. TODO: make this a function call instead? */
#define MIP_MSG_LEN sizeof(struct mip_msg) + msg->hdr.sdu_len * 4

/* Flags to use with `init_unix_sock()` */
#define USOCK_BIND 0x01
#define USOCK_LISTEN 0x02
#define USOCK_CONNECT 0x04

/* Printing functions */
void print_mac_addr(uint8_t * addr, size_t len);
void debug_print(struct ether_frame *eth,
                 struct mip_msg *msg, struct mip_cache *cache);

/* Socket initialization */
int init_raw_sock(void);
int init_unix_sock(char *socket_name, uint8_t flags);

/* Epoll operations */
int epoll_ctl_add(int epollfd, uint32_t events, int fd);
int epoll_ctl_mod(int epollfd, uint32_t events, int fd);
int epoll_ctl_del(int epollfd, int fd);

/* Sending and receiving raw packets */
int send_raw_packet(int sd,
                    uint8_t * buf,
                    size_t len,
                    struct ether_frame *frame_hdr, struct sockaddr_ll *so_addr);
struct mip_msg *recv_raw_packet(int sd,
                                struct ether_frame *frame_hdr,
                                struct sockaddr_ll *so_addr);

#endif                          /* _COMMON_H */
