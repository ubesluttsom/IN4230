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

int handle_mip_msg(struct mip_msg *, int, struct mip_cache *,
                   struct ether_frame *, struct sockaddr_ll *);
int init_epoll_table(int unix_sock, int raw_sock);
void mip_daemon(struct mip_cache *cache,
                int epollfd, int unix_sock, int raw_sock);
static void parse_cmd_opts(int argc, char *argv[]);
static void print_usage(char *);

#endif   /* _MIP_DAEMON_H */
