#ifndef _MIP_DAEMON_H
#define _MIP_DAEMON_H

#define MAX_EVENTS 10           /* max. number of concurrent events to check */

#include "mip.h"
#include "mip_arp.h"
#include "mip_unix.h"
#include "ether.h"
#include "common.h"
#include "queue.h"

/* Defining some return values */
#define MIP_MSG_TTL_Z -4

struct mip_daemon_fds {
        int epoll;         /* Epoll file descriptor */
        int connect;       /* Unix socket descriptor for new connections */
        int raw;           /* Raw socket descriptor */
        int router;        /* Router application socket */
        int ping;          /* Ping application socket */
};

struct mip_daemon_queue {
        struct queue *router;
        struct queue *arp;
};

/* Main loop */
void mip_daemon(
                struct mip_cache *cache,
                struct mip_daemon_fds *fds);

/* Dealing with MIP messages */

void handle_mip_unix_msg(
                struct mip_unix_msg *unix_msg,
                uint8_t sdu_type,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache,
                struct mip_daemon_queue *queue);
int handle_mip_msg(
                struct mip_msg *msg,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache,
                struct ether_frame *frame_hdr,
                struct sockaddr_ll *so_addr);
struct mip_msg *construct_mip_packet(
                uint8_t src_addr,
                struct mip_unix_msg *msg,
                uint8_t sdu_type);
int mip_application(
                struct mip_msg *msg,
                struct mip_daemon_fds *fds);
int mip_broadcast(
                struct mip_msg *msg,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache);
int mip_unicast(
                struct mip_msg *msg,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache);
int mip_send(
                struct mip_daemon_fds *fds,
                struct mip_msg *msg,
                struct mip_cache *cache_entry);

int send_router_request(
                uint8_t mip_addr,
                struct mip_daemon_fds *fds);
int send_router_response(
                uint8_t mip_addr,
                struct mip_daemon_fds *fds);


/* Queue management */

void handle_router_response(
                struct mip_unix_msg *res,
                struct mip_daemon_queue *queue,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache);

void process_queue(
                struct queue *queue,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache);

/* Program flow */
void handle_new_connection(
                struct mip_daemon_fds *fds);
void handle_router_daemon(
                struct mip_daemon_fds *fds,
                struct mip_cache *cache,
                struct mip_daemon_queue *queue);
void handle_ping_application(
                struct mip_daemon_fds *fds,
                struct mip_cache *cache,
                struct mip_daemon_queue *queue);
int handle_raw_sock(
                struct mip_daemon_fds *fds,
                struct mip_cache *cache,
                struct mip_daemon_queue *queue);

/* Small utilities */
int init_epoll_table(struct mip_daemon_fds *fds);
void close_fds(struct mip_daemon_fds *fds);
static void parse_cmd_opts(int argc, char *argv[]);
static void print_usage(char *cmd);

#endif                          /* _MIP_DAEMON_H */
