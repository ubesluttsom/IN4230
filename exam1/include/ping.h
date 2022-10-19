/*
 * Communication between MIP and application layer
 */

#ifndef PING_H
#define PING_H

/* Flags for pings and pongs */
#define PING 0x01
#define PONG 0x02

/* Maximum size of buffers sendt to and from MIP layer. I dislike setting this
 * explicitly like this, but the protocol makes it difficult otherwise (since
 * we're not also sending the buffer size with the ping)  */
#define BUF_SIZE 1450

/* Defining some return values */
#define SOCK_NOT_INIT -2
#define SOCK_CON_TERM -3

#include <string.h>             /* strncmp */
#include <sys/socket.h>         /* recv */
#include <stdio.h>              /* perror */
#include <errno.h>              /* errno */
#include <stdint.h>             /* uint8_t */

struct ping {
        uint8_t addr;
        char msg[BUF_SIZE];
};

int recv_ping(int sd, struct ping *ping, uint8_t type);
int send_ping(int sd, struct ping *ping);

#endif                          /* PING_H */
