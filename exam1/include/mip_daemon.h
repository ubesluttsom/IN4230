#ifndef _MIP_DAEMON_H
#define _MIP_DAEMON_H

#define MAX_EVENTS 10           /* max. number of concurrent events to check */

#include "mip.h"
#include "mip_arp.h"
#include "ether.h"
#include "common.h"
#include "ping.h"

/* Defining some return values */
#define MIP_MSG_TTL_Z -4

struct mip_pkt_queue {
        struct mip_pkt_queue *next;
        struct mip_msg *msg;
};

/* Main loop */
void mip_daemon(struct mip_cache *cache, int epollfd, int unix_sock,
                int raw_sock);

/* Dealing with MIP messages */
int handle_mip_msg(struct mip_msg *msg, int unix_sock, struct mip_cache *cache,
                   struct ether_frame *frame_hdr, struct sockaddr_ll *so_addr);
int handle_ping_packet(struct mip_msg *msg, int unix_sock,
                       struct mip_cache *cache);
struct mip_msg *construct_mip_ping(uint8_t src_addr, uint8_t dst_addr,
                                   uint8_t * buf);

/* Queue management */
void add_to_queue(struct mip_pkt_queue *queue, struct mip_msg *msg);
void process_queue(struct mip_pkt_queue *queue, struct mip_cache *cache);

/* Program flow */
int handle_new_connection(int epollfd, int unix_sock);
void handle_application(int epollfd, int *app_sd, struct epoll_event events[],
                        struct mip_cache *cache, struct mip_pkt_queue *queue);
int handle_raw_sock(int epollfd, int raw_sock, int *app_sd,
                    struct mip_cache *cache);

/* Small utilities */
int init_epoll_table(int unix_sock, int raw_sock);
static void parse_cmd_opts(int argc, char *argv[]);
static void print_usage(char *cmd);

#endif                          /* _MIP_DAEMON_H */
