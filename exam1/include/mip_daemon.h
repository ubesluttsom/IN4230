/*
 * Check https://valecs.gitlab.io/resources/CHeaderFileGuidelines.pdf for some
 * more nice practises.
 */

#ifndef _MIP_DAEMON_H
#define _MIP_DAEMON_H

#include "mip.h"
#include "mip_arp.h"
#include "ether.h"
#include "common.h"

#include <unistd.h>   /* getopt() */

struct mip_pkt_queue {
        struct mip_pkt_queue *next;
        struct mip_msg *msg;
};

int handle_mip_msg(uint8_t *, int, struct mip_cache *,
                   struct ether_frame *, struct sockaddr_ll *);
int init_epoll_table(int unix_sock, int raw_sock);
void parse_cmd_opts(int argc, char *argv[]);
void print_usage(char *);
void mip_daemon(struct mip_cache *cache,
                int epollfd, int unix_sock, int raw_sock);

#endif   /* _MIP_DAEMON_H */
