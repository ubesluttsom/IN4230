#ifndef _MIP_ROUTER_H
#define _MIP_ROUTER_H

#include <stdio.h>              /* printf */
#include <stdlib.h>             /* exit */
#include <unistd.h>             /* close */
#include <string.h>             /* memset */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>         /* sockets operations */
#include <sys/param.h>          /* MAX() and MIN() */
#include <stdbool.h>            /* boolean types */
#include <fcntl.h>              /* O_NONBLOCK */

#include "mip_unix.h"
#include "common.h"
/* ROUTING TABLE STRUCTURE */

#define MAX_VAL 0xFF

struct dist_vec {
        uint8_t src;                 /* Mip associated with this dist vec */
        uint8_t dst[MIP_BROADCAST];  /* Cost to reach each mip from `src` */
};

struct routing_table {
        int n;
        struct dist_vec *via;
};

/* The variable `n` contain the number of neighboring nodes, this is also the
 * length of the flexible array member. `cost_via` should be a list of the
 * neighboring MIP addresses. And `dst` is all possible destination
 * MIP-addresses. Each entry in the table is the cost of reaching `dst` via the
 * address in the `cost_via` column.
 *
 * Example:
 *         Routing table for node 001. This node can only reach 1 MIP node
 *         (002):
 *         
 *                              dst
 *         
 *                      mip│001│002│003│004│005│...
 *                      ───┼───┼───┼───┼───┼───┼───
 *                 via  001│ 0 │ 1 │ 2 │ 2 │ 3 │
 *         
 *         Routing Table for node 002. This node can reach 3 MIP nodes: 001,
 *         003, and 004:
 *         
 *                            dst
 *         
 *                     mip│001│003│004│005│...
 *                     ───┼───┼───┼───┼───┼───
 *                     001│ 1 │inf│inf│inf│
 *                     ───┼───┼───┼───┼───┼───
 *                via  003│inf│ 1 │ 2 │ 3 │
 *                     ───┼───┼───┼───┼───┼───
 *                     004│inf│ 2 │ 1 │ 2 │
 */        

/* Main program loop */

void router(char *socket_name);

/* Program flow */

int handle_message(
                int sd,
                struct dist_vec *mdv,
                struct routing_table *rt);
int handle_update(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt);
int handle_request(
                int sd,
                struct mip_unix_msg *msg,
                struct routing_table *rt);
int handle_response(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt);
int handle_hello(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt);

/* Sending data */

int send_hello(int sd);
int send_update(uint8_t mip_addr, int sd, struct dist_vec *mdv, struct routing_table *rt);
struct dist_vec poison_reverse(struct dist_vec *mdv, uint8_t mip_addr, struct routing_table *rt);

/* Managing the routing table */

struct routing_table *init_routing_table();
void free_routing_table(struct routing_table *rt);
struct dist_vec *expand_routing_table(struct routing_table *rt, uint8_t mip_addr);
bool update_routing_table(struct routing_table *rt, struct dist_vec *new_vec);

struct dist_vec *get_dist_vec(uint8_t mip_addr, struct routing_table *rt);

struct dist_vec *init_mdv();
bool update_mdv(struct routing_table *rt, struct dist_vec *mdv);

/* Lookup */

uint8_t query(uint8_t mip_addr, struct routing_table *rt);

/* Small utilities */

static void print_usage(char *cmd);
static void parse_cmd_opts(int argc, char *argv[]);

#endif /* _MIP_ROUTER_H */
