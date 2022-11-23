#include "mip_router.h"

/* Debug flag. Using global, since it is set once, then never changed. */
static int d = 0;

/* Mip address of *this* node */
static int self = 0;

void router(char *socket_name)
{
        struct epoll_event events[1];
        int sd, epollfd, rc;
        struct timeval timeout;

        struct routing_table *rt;   /* Routing table for this node */
        struct dist_vec *mdv;       /* Minimal Distance Vector */

        /* Initialize the routing table and the minimal distance vector */
        rt = init_routing_table();
        mdv = init_mdv();

        /* Connect to MIP daemon on layer below */
        sd = mip_connect(socket_name, MIP_T_ROUTING);
        if (sd < 0)
                goto close_sd;

        /* Set a timeout (1 sec) for daemon to send back it's mip address */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        /* Recv address of *this* node */
        if ((recv(sd, &self, sizeof(uint8_t), 0)) != sizeof(uint8_t)) {
                printf("<mip_router: error: connection fail, address expected>\n");
                goto close_sd;
        }

        /* Create the main epoll file descriptor */
        epollfd = epoll_create1(0);
        if (epollfd < 0) {
                perror("epoll_create1");
                goto close_sd;
        }

        /* Add the main listening socket to epoll table */
        rc = epoll_ctl_add(epollfd, EPOLLIN, sd);
        if (rc < 0)
                goto close_epoll;

        /* Set socket to be non-blocking */
        fcntl(sd, F_SETFL, fcntl(sd, F_GETFL) | O_NONBLOCK);

        /* Main loop */
        do {
                /* Timeout reached */
                if (rc == 0) {
                        if (send_hello(sd) < 0)
                                goto close_epoll;

                /* Handle event on socket */
                } else if (events->data.fd == sd) {
                        if (handle_message(sd, mdv, rt) < 0)
                                goto close_epoll;
                }

                /* Wait for 10 seconds */
        } while ((rc = epoll_wait(epollfd, events, 1, 10000)) >= 0);

        if (rc == -1)
                perror("epoll_wait");

close_epoll:
        close(epollfd);
close_sd:
        close(sd);
        free_routing_table(rt);
        exit(EXIT_FAILURE);
}

/* PROGRAM FLOW */

int handle_message(
                int sd,
                struct dist_vec *mdv,
                struct routing_table *rt)
{
        int rc;
        struct mip_unix_msg msg;

        /* Try to read message in socket */
        rc = recv(sd, &msg, sizeof(struct mip_unix_msg), 0);

        /* Disgard malformed messages */
        if (rc < 0) {
                perror("recv");
                return -1;
        } else if (rc == 0) {
                if (d) printf("<mip_router: connection to daemon closed>\n");
                return -1;
        }

        switch (msg.routing.type[2]) {

        case 'L': /* MIP_R_HE'L' */
                return handle_hello(sd, &msg, mdv, rt);

        case 'D': /* MIP_R_UP'D' */
                return handle_update(sd, &msg, mdv, rt);

        case 'Q': /* MIP_R_RE'Q' */
                return handle_request(sd, &msg, rt);

        case 'P': /* MIP_R_RS'P' */
                return handle_response(sd, &msg, mdv, rt);

        default:
                if (d) printf("<mip_router: error: unknown router message>\n");
                return -1;
        }
}

int handle_update(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt)
{
        struct dist_vec *new_vec;

        if (d) printf("<mip_router: recived update>\n");

        /* Cast routing message payload to a distance vector. */
        new_vec = (struct dist_vec *)msg->routing.msg;

        /* Set interface address */
        new_vec->src = msg->addr;

        /* Update the routing table and minimal distance vector */
        if (update_routing_table(rt, new_vec))
                if (update_mdv(rt, mdv))
                        /* If the updated routing table also updates the
                         * minimal distance vector, reply with an UPDate
                         * broadcast */
                        return send_update(MIP_BROADCAST, sd, mdv, rt);
        return 0;
}

int handle_request(
                int sd,
                struct mip_unix_msg *req,
                struct routing_table *rt)
{
        struct mip_unix_msg rsp;

        /* Initialize RESPONSE message structure */
        memset(&rsp, '\0', sizeof(struct mip_unix_msg));
        rsp.addr = req->addr;
        rsp.ttl = 1;
        memcpy(&rsp.routing.type, &"RSP", strlen("RSP"));

        /* Set response address with a query to the routing table */
        rsp.routing.addr = query(req->routing.addr, rt);

        /* Try to send RESPONSE */
        if (d) printf("<mip_router: sending response>\n");
        return send_data(sd, &rsp, sizeof(struct mip_unix_msg));
}

int handle_response(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt)
{
        /* A response from the daemon is interpreted as a downed link */

        /* Set cost of routing through responded address to MAX_VAL */
        for (int i = 0; i < rt->n; i++)
                rt->via[i].dst[msg->routing.addr] = MAX_VAL;

        /* Update minimal distance vector */
        if (update_mdv(rt, mdv)) {
                /* Send update *broadcast*, if MDV is changed */
                send_update(MIP_BROADCAST, sd, mdv, rt);
                send_hello(sd);
        }
        
        return 0;
}

int handle_hello(
                int sd,
                struct mip_unix_msg *msg,
                struct dist_vec *mdv,
                struct routing_table *rt)
{
        struct dist_vec *dv;

        if (d) printf("<mip_router: recived hello>\n");
        
        /* If MIP address of HELlo is not in routing table, expand it */
        if (!(dv = get_dist_vec(msg->addr, rt)))
                dv = expand_routing_table(rt, msg->addr);

        /* Set distance to node that sent HELLO to be 1 */
        dv->dst[msg->addr] = 1;

        /* Update minimal distance vector */
        if (update_mdv(rt, mdv))
                /* Reply with update *broadcast*, if MDV is changed */
                return send_update(MIP_BROADCAST, sd, mdv, rt);
        else
                /* Reply with update *unicast*, if MDV is not changed */
                return send_update(msg->addr, sd, mdv, rt);
}

/* SENDING DATA */

int send_hello(int sd)
{
        struct mip_unix_msg msg;

        /* Initialize HELLO message structure */
        memset(&msg, '\0', sizeof(struct mip_unix_msg));
        msg.addr = MIP_BROADCAST;
        msg.ttl = 1;
        memcpy(&msg.routing.type, &"HEL", strlen("HEL"));

        /* Try to send HELLOs */
        if (d) printf("<mip_router: sending hellos>\n");
        return send_data(sd, &msg, sizeof(struct mip_unix_msg));
}

int send_update(uint8_t mip_addr,
                int sd,
                struct dist_vec *mdv,
                struct routing_table *rt)
{
        struct dist_vec dv;
        struct mip_unix_msg msg;

        /* Initialize UPD message structure */
        memset(&msg, '\0', sizeof(struct mip_unix_msg));
        msg.ttl = 1;
        memcpy(&msg.routing.type, &"UPD", strlen("UPD"));

        /* Try to send UPDate */
        if (d) printf("<mip_router: sending updates>\n");

        /* If broadcast, send on each neighboring node (that are in our table);
         * if unicast to spesific mip address, limit for-loop. */
        for (int i = 0; i < rt->n; i++) {
                if (mip_addr != MIP_BROADCAST && rt->via[i].src != mip_addr)
                        continue;

                /* Fill in a poisoned MDV */
                dv = poison_reverse(mdv, rt->via[i].src, rt);

                /* Cost of sending to self is always 0 */
                dv.dst[self] = 0;

                /* Copy over modified distance vector `dv` and send message */
                msg.addr = rt->via[i].src;
                memcpy(&msg.routing.msg, &dv, sizeof(struct dist_vec));
                if (send_data(sd, &msg, sizeof(struct mip_unix_msg)) < 0)
                        return -1;

                if (mip_addr != MIP_BROADCAST)
                        break;
        }

        return 0;
}

struct dist_vec poison_reverse(
                struct dist_vec *mdv,
                uint8_t mip_addr,
                struct routing_table *rt)
{
        struct dist_vec dv = *mdv;

        /* Poison reverse: if a node N uses interface `mip_addr` to transmit to
         * a given destination `dst`, N sends through `mip_addr` the
         * information that its cost-to-go to `dst` is infinite. */
        for (int dst = 0; dst < MIP_BROADCAST; dst++)
                if (query(dst, rt) == mip_addr)
                        dv.dst[dst] = MAX_VAL;
        return dv;
}

/* MANAGING THE ROUTING TABLE */

struct routing_table *init_routing_table()
{
        /* Return a empty routing table. `calloc()` sets `rt->n = 0` for us. */
        return calloc(1, sizeof(struct routing_table));
}

void free_routing_table(struct routing_table *rt)
{
        free(rt->via);
        free(rt);
}

struct dist_vec *expand_routing_table(struct routing_table *rt, uint8_t mip_addr)
{
        /* Rellocate memory to new size */
        rt->n += 1;
        rt->via = realloc(rt->via, rt->n * sizeof(struct dist_vec));

        /* Set correct source address for new router table vector */
        rt->via[rt->n - 1].src = mip_addr;

        /* Set new memory to max values */
        for (int i = 0; i < MIP_BROADCAST; i++)
                rt->via[rt->n - 1].dst[i] = MAX_VAL;

        /* Return a pointer the the newly allocated distance vector */
        return &rt->via[rt->n - 1];
}

bool update_routing_table(struct routing_table *rt, struct dist_vec *new_vec)
{
        struct dist_vec *old_vec;
        bool is_changed;

        is_changed = false;

        /* Find the relevant entry in routing table */
        old_vec = get_dist_vec(new_vec->src, rt);

        /* If, for some reason, the address has no entry, create room for it */
        if (!old_vec)
                old_vec = expand_routing_table(rt, new_vec->src);

        /* Adjust all value in updated new distance vector */
        for (int i = 0; i < MIP_BROADCAST; i++)
                /* Increment if less than max value */
                if (new_vec->dst[i] < MAX_VAL)
                        new_vec->dst[i]++;
 
        /* Cost of sending to self through this node */
        new_vec->dst[self] = MAX_VAL;

        /* Update corresponding table with new costs, if they are different */
        if (memcmp(old_vec, new_vec, sizeof(struct dist_vec)) != 0) {
                *old_vec = *new_vec;
                is_changed = true;
        }

        return is_changed;
}

struct dist_vec *get_dist_vec(uint8_t mip_addr, struct routing_table *rt)
{
        /* Find corresponding distance vector (row) in table */
        for (int i = 0; i < rt->n; i++)
                if (rt->via[i].src == mip_addr)
                        return &rt->via[i];

        /* Return NULL if `mip_addr` is not in table */
        return NULL;
}

struct dist_vec *init_mdv()
{
        struct dist_vec *mdv;

        /* Allocate a new distance vector. Set to maximum values */
        mdv = malloc(sizeof(struct dist_vec));

        /* Set own address */
        mdv->src = self;

        /* Set new memory to max values */
        for (int i = 0; i < MIP_BROADCAST; i++)
                mdv->dst[i] = MAX_VAL;

        return mdv;
}

bool update_mdv(struct routing_table *rt, struct dist_vec *mdv)
{
        bool is_changed = false;

        /* Fill inn MDV with shortest routes. I.e, for all neighboring nodes,
         * pick the one with the lowest cost. */
        for (int i = 0; i < MIP_BROADCAST; i++) {
                for (int j = 0; j < rt->n; j++)
                        if (rt->via[j].dst[i] < mdv->dst[i]) {
                                mdv->dst[i] = rt->via[j].dst[i];
                                is_changed = true;
                        }
        }

        return is_changed;
}

/* LOOKUP */

uint8_t query(uint8_t mip_addr, struct routing_table *rt)
{
        int route_to, i;

        /* Find least cost interface to send, for given MIP address. Default to
         * broadcast address if no good forwarding address is found */

        for (i = 0, route_to = 0; i < rt->n; i++)
                if (rt->via[i].dst[mip_addr] < rt->via[route_to].dst[mip_addr])
                        route_to = i;

        if (rt->via[route_to].dst[mip_addr] == MAX_VAL)
                return MIP_BROADCAST;
        else 
                return rt->via[route_to].src;
}

/* SMALL UTILITIES */

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] [-d] <socket_lower>\n", cmd);
}

static void parse_cmd_opts(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "hd")) != -1) {
                switch (opt) {
                case 'h':
                        print_usage(argv[0]);
                        exit(EXIT_SUCCESS);

                case 'd':
                        /* Global debug flag */
                        d = 1;
                        continue;
                }
        }

        if (argc < 2 || argc > 3) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

/* MAIN */

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        router(argv[argc - 1]);
}

