/*
 * Communication between MIP and application layer
 */

#ifndef MIP_UNIX_H
#define MIP_UNIX_H

/* Flags for pings and pongs */
#define PING 0x01
#define PONG 0x02

/* Maximum size of buffers send to and from MIP layer. I dislike setting this
 * explicitly like this, but the protocol makes it difficult otherwise (since
 * we're not also sending the buffer size with the ping). Also: the maximum
 * length of a Ethernet frame payload is 1500; this need to be considered. */
#define BUF_SIZE 1450

/* TTL value to default to if not set explicitly */
#define TTL_DEFAULT 7

/* Defining some return values */
#define SOCK_NOT_INIT -2
#define SOCK_CON_TERM -3

/* Flags to use with `init_unix_sock()` */
#define USOCK_BIND 0x01
#define USOCK_LISTEN 0x02
#define USOCK_CONNECT 0x04

#define MAX_CONNS 5          /* max. length of the pending connections queue */

#include <string.h>             /* strncmp */
#include <sys/socket.h>         /* recv */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <stdio.h>              /* perror */
#include <errno.h>              /* errno */
#include <stdint.h>             /* uint8_t */
#include <unistd.h>             /* close */

/* Generic message over Unix socket
 * TODO: I'm trying a little experiment with a mix of nested unions and structs
 *       It seems to work fine ... Explanation: the struct is always the same
 *       total length, but the `union` lets you choose if you want to view it
 *       as a generic `msg` or a `routing` message.
 */
struct mip_unix_msg {
        uint8_t addr;
        uint8_t ttl;
        union {
                char msg[BUF_SIZE];
                struct {
                        uint8_t type[3];
                        union {
                                uint8_t addr;
                                uint8_t msg[BUF_SIZE-3];
                        };
                } routing;
        };
};

/* MIP router routing types */
#define MIP_R_HEL 0x48454C
#define MIP_R_UPD 0x555044
#define MIP_R_REQ 0x524551
#define MIP_R_RSP 0x525350

int init_unix_sock(char *socket_name, uint8_t flags);
int mip_connect(char *sock_path, uint8_t sdu_type);
int recv_ping(int sd, struct mip_unix_msg *ping, uint8_t type);
int send_ping(int sd, struct mip_unix_msg *ping);
int send_sdu_type(int unix_sock, uint8_t type);
int send_data(int unix_sock, const void *data, size_t len);

#endif                          /* MIP_UNIX_H */
