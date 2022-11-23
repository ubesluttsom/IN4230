#include "mip_daemon.h"

/* Debug flag. Using global, since it is set once, then never changed. */
static int d = 0;

/* MIP address of *this* node */
static int self = 0;

/* MAIN */

int main(int argc, char *argv[])
{
        char *connect_path;     /* Path to new-connection socket */

        struct mip_cache cache; /* List of all network interfaces */

        struct mip_daemon_fds fds;          /* File descriptor storage */

        /* Parse command line options and arguments */
        parse_cmd_opts(argc, argv);
        self = atoi(argv[argc - 1]);
        connect_path = argv[argc - 2];

        /* Unlink the socket so that we can reuse the program. This is bad
           hack! Better solution with a lock file, or signal handling. Check
           https://gavv.github.io/articles/unix-socket-reuse. FIXME! */
        unlink(connect_path);

        fds.router = -1;
        fds.ping = -1;

        /* Initialize sockets */
        fds.connect = init_unix_sock(connect_path, USOCK_BIND | USOCK_LISTEN);
        fds.raw = init_raw_sock();
        if (fds.raw == -1 || fds.connect == -1)
                goto close_socks;
        fds.epoll = init_epoll_table(&fds);
        if (fds.epoll == -1)
                goto close_socks;


        /* Initialize the MIP-ARP cache */
        init_mip_cache(self, &cache, fds.raw);

        /* Run the main program loop */
        mip_daemon(&cache, &fds);

        return EXIT_SUCCESS;

close_socks:
        close_fds(&fds);
        exit(EXIT_FAILURE);
}

void mip_daemon(struct mip_cache *cache,
                struct mip_daemon_fds *fds)
{
        struct epoll_event events[MAX_EVENTS];  /* Storage of epoll table */

        /* Queue for storing packages waiting to be processed. They get added
           after router queries, and/or MIP-ARP cache misses */
        struct mip_daemon_queue queue = { queue_create(), queue_create() };

        /* Wait for something to happen on any socket */
        while (epoll_wait(fds->epoll, events, MAX_EVENTS, -1)) {

                /* Handle incoming application connection */
                if (events->data.fd == fds->connect) {
                        handle_new_connection(fds);

                /* Receive data from raw socket */
                } else if (events->data.fd == fds->raw) {
                        handle_raw_sock(fds, cache, &queue);

                        /* Try to send packets in the queue after recieving raw
                           packets, since they might have updated the cache */
                        process_queue(queue.arp, fds, cache);

                /* Ping application is trying to send data */
                } else if (events->data.fd == fds->ping) {
                        handle_ping_application(fds, cache, &queue);

                /* Router daemon is trying to send data */
                } else if (events->data.fd == fds->router) {
                        handle_router_daemon(fds, cache, &queue);

                } else {
                        /* ??? */
                        printf("Unknown error\n");
                        goto close_socks;
                }
        }

        perror("epoll_wait");

 close_socks:
        queue_flush(queue.arp);
        queue_flush(queue.router);
        close_fds(fds);
        exit(EXIT_FAILURE);
}

/* PROGRAM FLOW */

void handle_router_daemon(struct mip_daemon_fds *fds,
                          struct mip_cache *cache,
                          struct mip_daemon_queue *queue)
{
        /*
         * XXX UNDER CONSTRUCTION XXX
         */

        int rc;
        struct mip_unix_msg unix_msg;
        struct mip_msg *msg;

        if (d) printf("<mip_daemon: recv'd from router app>\n");

        rc = recv(fds->router, &unix_msg, sizeof(struct mip_unix_msg), 0);
        if (rc <= 0)
                goto epoll_del;

        /* Check router message type. Be lazy and just check last `char`, since
         * it's the only one different in all cases. */
        switch (unix_msg.routing.type[2]) {

        case 'L': /* MIP_R_HE'L' */
        case 'D': /* MIP_R_UP'D' */

                /* Process as a generic Unix message */
                /* handle_mip_unix_msg(&msg, MIP_T_ROUTING, fds, cache, queue); */

                /* Make a MIP packet out of the Unix message */
                msg = construct_mip_packet(cache->mip, &unix_msg, MIP_T_ROUTING);
                if (!msg)
                        return;

                /* Process MIP packet */
                if (handle_mip_msg(msg, fds, cache, NULL, NULL) == MIP_CACHE_MISS) {

                        /* Send arp request, then push message into arp-queue, awaiting
                         * processing after reply. */
                        send_arp_request(msg->hdr.dst_addr, cache);
                        queue_tail_push(queue->arp, msg);

                } else
                        free(msg);

                break;

        case 'P': /* MIP_R_RS'P' */

                handle_router_response(&unix_msg, queue, fds, cache);
                break;

        case 'Q': /* MIP_R_RE'Q' */
                  /* This should never happen. A request from the routing
                   * deamon? */
        default:
                if (d) printf("<mip_daemon: error: unknown router message>\n");
                return;
        }

        return;

 epoll_del:
        if (d) printf("<mip_daemon: router disconnected>\n");
        epoll_ctl_del(fds->epoll, fds->router);
        fds->router = -1;
        return;
}

void handle_ping_application(struct mip_daemon_fds *fds,
                             struct mip_cache *cache,
                             struct mip_daemon_queue *queue)
{
        struct mip_unix_msg ping;

        /* Try to read from the application */
        if (recv_ping(fds->ping, &ping, PING | PONG) <= 0)
                /* No bytes read. Connection likely terminated */
                goto epoll_del;

        /* Process Unix message */
        handle_mip_unix_msg(&ping, MIP_T_PING, fds, cache, queue);

        return;

 epoll_del:
        if (d) printf("<mip_daemon: ping app disconnected>\n");
        epoll_ctl_del(fds->epoll, fds->ping);
        fds->ping = -1;
}

void handle_mip_unix_msg(struct mip_unix_msg *unix_msg,
                         uint8_t sdu_type,
                         struct mip_daemon_fds *fds,
                         struct mip_cache *cache,
                         struct mip_daemon_queue *queue)
{
        struct mip_msg *msg;

        /* Make a MIP packet out of the Unix message */
        msg = construct_mip_packet(cache->mip, unix_msg, sdu_type);
        if (!msg)
                return;

        /* Process MIP packet */
        if (handle_mip_msg(msg, fds, cache, NULL, NULL) == MIP_CACHE_MISS) {

                /* If recipient is not in cache, query the router for
                 * forwarding address. Then add message to router queue, to be
                 * processed after response. */
                if (send_router_request(msg->hdr.dst_addr, fds) > 0) {
                        queue_tail_push(queue->router, msg);
                        return;
                }
        }

        /* Free MIP message */
        free(msg);
        return;
}

int send_router_request(
                uint8_t mip_addr,
                struct mip_daemon_fds *fds)
{
        struct mip_unix_msg req = { 0 };

        /* Initialize REQUEST message structure */
        req.addr = self;                              /* Host mip address */
        req.ttl = 1;                           /* FIXME: remove hardcoded */
        memcpy(&req.routing.type, &"REQ", strlen("REQ"));

        /* Set requested address */
        req.routing.addr = mip_addr;

        /* Try to send REQUEST */
        if (d) printf("<mip_daemon: sending request>\n");
        return send_data(fds->router, &req, sizeof(struct mip_unix_msg));
}

int send_router_response(
                uint8_t mip_addr,
                struct mip_daemon_fds *fds)
{
        /* This function is used to notify the router of downed links. It would
         * probably be better to design another SDU type for this ... */

        struct mip_unix_msg res = { 0 };

        /* Initialize RESPONSE message structure */
        res.addr = self;                              /* Host mip address */
        res.ttl = 1;                           /* FIXME: remove hardcoded */
        memcpy(&res.routing.type, &"RES", strlen("RES"));

        /* Set RESPONSE address */
        res.routing.addr = mip_addr;

        /* Try to send RESPONSE */
        if (d) printf("<mip_daemon: router notified of downed link>\n");
        return send_data(fds->router, &res, sizeof(struct mip_unix_msg));
}


int handle_raw_sock(struct mip_daemon_fds *fds,
                    struct mip_cache *cache,
                    struct mip_daemon_queue *queue)
{
        struct sockaddr_ll so_addr;                       /* Address storage */
        struct ether_frame frame_hdr;              /* Ethernet frame storage */
        struct mip_msg *msg;                          /* MIP message pointer */

        /* Get message from raw socket */
        msg = recv_raw_packet(fds->raw, &frame_hdr, &so_addr);
        if (msg == NULL)
                return -1;

        /* Confirm right protocol */
        if (frame_hdr.eth_proto != htons(ETH_P_MIP)) {
                if (d) printf("<mip_daemon: unknown Ethernet protocol>\n");
                free(msg);
                return -1;
        }

        if (d) printf("<mip_daemon: mip recv'd from %d>\n", msg->hdr.src_addr);

        /* Process packet */
        switch (handle_mip_msg(msg, fds, cache, &frame_hdr, &so_addr)) {

        case MIP_CACHE_MISS:

                /* Forward message. Look up forwarding address */
                if (send_router_request(msg->hdr.dst_addr, fds) > 0) {
                        queue_tail_push(queue->router, msg);
                        return 0;
                } else
                        break;

        case SOCK_CON_TERM:
                /* The socket connection is terminated. */
        case SOCK_NOT_INIT:
                /* The socket connection is not initialized.*/
        case MIP_MSG_TTL_Z:
                /* The TTL is zero. */
        default:
                /* Discard message */ 
                break;
        }

        free(msg);
        return 0;
}

void handle_new_connection(struct mip_daemon_fds *fds)
{
        int app_sd;
        uint8_t sdu_t;
        struct timeval timeout;

        app_sd = accept(fds->connect, NULL, NULL);
        if (app_sd < 0) {
                perror("accept");
                return;
        }

        /* Set a timeout (1 sec) for application to send SDU type identifier */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(app_sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        /* Recv SDU type identifier */
        if ((recv(app_sd, &sdu_t, sizeof(uint8_t), 0)) != sizeof(uint8_t)) {
                printf("<mip_daemon: error: connection fail, SDU expected>\n");
                close(app_sd);
                return;
        }

        /* Add new socket descriptor to epoll table */
        if (epoll_ctl_add(fds->epoll, EPOLLIN, app_sd) < 0)
                goto close_app_sd;    /* TODO: Fancier error handling? */

        switch (sdu_t) {
        case MIP_T_PING:
                fds->ping = app_sd;
                break;
        case MIP_T_ROUTING:
                fds->router = app_sd;

                /* Send back own mip address */
                send_data(fds->router, &self, sizeof(uint8_t));

                break;
        default:
                printf("<mip_daemon: error: unknown SDU type>\n");
                goto close_app_sd;
        }

        if (d) printf("<mip_daemon: application connected>\n");

        return;

close_app_sd:
        close(app_sd);
        return;
}

/* DEALING WITH MIP MESSAGES */

int handle_mip_msg(struct mip_msg *msg,
                   struct mip_daemon_fds *fds,
                   struct mip_cache *cache,
                   struct ether_frame *frame_hdr,
                   struct sockaddr_ll *so_addr)
{
        int rc;
        uint8_t dst = msg->hdr.dst_addr;  /* Destination MIP-address */

        /* Be strict and check message type. View this as a filter */
        switch (msg->hdr.sdu_type) {

        case MIP_T_ARP:

                /* If the message is of type ARP, handle specially */
                rc = handle_arp_packet(msg, cache, frame_hdr, so_addr);

                /* Quietly drop arp cache misses. Never forward them, or put
                 * them in the queue. */
                if (rc == MIP_CACHE_MISS)
                        return 0;
                else
                        return rc;

        case MIP_T_PING:
        case MIP_T_ROUTING:

                break;   /* Message has a valid type. Jump to logic below */

        default:
                if (d) printf("<mip_daemon: error: unknown MIP SDU type>\n");
                return -1;
        }

        /* If the recipient is *this* node, or if received message is broadcast
         * on raw socket (incoming), pass to application. The if-test is a bit
         * of a hack, and non-intuitive: `frame_hdr && so_addr` is True iff the
         * message was received on raw socket. As a consequence of this check,
         * broadcast packets are *never* forwarded (which is good). */
        if ((dst == self) || ((dst == MIP_BROADCAST) && frame_hdr && so_addr))
                return mip_application(msg, fds);

        /* Drop the package if TTL has expired */
        if (msg->hdr.ttl == 0)
                return MIP_MSG_TTL_Z;

        /* Finally, forward packet */
        if (dst == MIP_BROADCAST)
                return mip_broadcast(msg, fds, cache);
        else
                return mip_unicast(msg, fds, cache);
}

int mip_unicast(struct mip_msg *msg,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache)
{
        struct mip_cache *ptr;

        /* Else, try to find correct entry in MIP cache */
        ptr = mip_cache(msg->hdr.dst_addr, cache);

        /* If destination address is *not* in cache */
        if (ptr == NULL)
                return MIP_CACHE_MISS;

        /* Send packet as unicast */
        mip_send(fds, msg, ptr);

        /* Print sent MIP message */
        if (d) printf("<mip_daemon: mip message sent to %d>\n",
                        msg->hdr.dst_addr);
        /* if (d) debug_print(&ether_hdr, msg, cache_entry); */

        return 0;
}

int mip_broadcast(struct mip_msg *msg,
                         struct mip_daemon_fds *fds,
                         struct mip_cache *cache)
{

        /* Send raw packet, broadcast. I.e, send on all interfaces in cache */
        for (; cache->next != NULL; cache = cache->next)
                mip_send(fds, msg, cache->next);

        /* Print sent MIP message */
        if (d) printf("<mip_daemon: mip message sent to %d>\n",
                        msg->hdr.dst_addr);
        /* if (d) debug_print(&ether_hdr, msg, cache_entry); */

        return 0;
}

int mip_send(struct mip_daemon_fds *fds,
             struct mip_msg *msg,
             struct mip_cache *cache_entry)
{
        int rc;
        struct ether_frame ether_hdr;

        /* Initialize Ethernet header */
        memcpy(&ether_hdr.dst_addr, cache_entry->mac, ETH_MAC_LEN);
        memcpy(&ether_hdr.src_addr, cache_entry->addr.sll_addr, ETH_MAC_LEN);
        ether_hdr.eth_proto = htons(ETH_P_MIP);

        /* Decrement TTL before sending */
        msg->hdr.ttl = msg->hdr.ttl - 1;

        /* #define MIP_MSG_LEN sizeof(struct mip_msg) + msg->hdr.sdu_len * 4 */

        /* Try to send packet through raw socket */
        rc = send_raw_packet(fds->raw, (uint8_t *) msg, MIP_MSG_LEN,
                             &ether_hdr, &cache_entry->addr);
        if (rc == -1) {
                perror("send_raw_packet");
                return rc;
        }

        if (errno == ENETDOWN) {
                send_router_response(cache_entry->mip, fds);
        }

        return rc;
}

int mip_application(struct mip_msg *msg, struct mip_daemon_fds *fds)
{
        int fd;

        /* Handle the different types of MIP package */
        switch (msg->hdr.sdu_type) {

        case MIP_T_PING:
                fd = fds->ping;
                break;

        case MIP_T_ROUTING:
                fd = fds->router;
                break;

        default:
                if (d) printf("<mip_daemon: error: unknown MIP SDU type>\n");
                return -1;
        }

        struct mip_unix_msg unix_msg;

        /* Construct the message to be passed over the unix socket */
        memset(&unix_msg, '\0', sizeof(struct mip_unix_msg));
        memcpy(&unix_msg.addr, &msg->hdr.src_addr, sizeof(uint8_t));

        unix_msg.addr = msg->hdr.src_addr;
        unix_msg.ttl = msg->hdr.ttl;

        /* Make sure no buffer is overrun */
        if (msg->hdr.sdu_len * 4 > sizeof(unix_msg.msg))
                memcpy(&unix_msg.msg, msg->sdu, sizeof(unix_msg.msg));
        else
                memcpy(&unix_msg.msg, msg->sdu, msg->hdr.sdu_len * 4);

        /* Try to send it to application */
        return send_data(fd, &unix_msg, sizeof(struct mip_unix_msg));
}

struct mip_msg *construct_mip_packet(
                uint8_t src_addr,
                struct mip_unix_msg *unix_msg,
                uint8_t sdu_type)
{
        /* FIXME: This should be serialized. */

        struct mip_msg *msg;
        struct mip_hdr hdr = {0};

        /* Prepare MIP message. Set address, protocol, and TTL */
        hdr.dst_addr = unix_msg->addr;
        hdr.src_addr = src_addr;
        hdr.sdu_type = sdu_type;

        /* Set TTL. Use default value if 0. */
        hdr.ttl = (unix_msg->ttl == 0) ? TTL_DEFAULT : unix_msg->ttl;

        /* Length is calculated by rounding up the integer division; C
           truncates the division, so I use this neat math trick: */
        if (sdu_type == MIP_T_ROUTING)
                /* FIXME: The length determination here is dumb. It is just set
                 * to the highest possible number. Is it possible to use the
                 * `rc` from the `recv()` call instead? */
                hdr.sdu_len = (sizeof(struct mip_unix_msg) + (4 - 1)) / 4;
        else    /* MIP_T_PING */
                hdr.sdu_len = (strlen((char *)unix_msg->msg) + (4 - 1)) / 4;

        if (sizeof(struct mip_hdr) + hdr.sdu_len * 4 < ETH_PAYLOAD) {
                /* Allocate memory for message, and attach header */
                msg = calloc(1, ETH_PAYLOAD);
        } else {
                if (d) printf("<mip_daemon: error: SDU too long!>\n");
                return NULL;
        }

        /* Fill message */
        memcpy(&msg->hdr, &hdr, sizeof(struct mip_hdr));
        memcpy(msg->sdu, unix_msg->msg, hdr.sdu_len * 4);

        return msg;
}

/* QUEUE MANAGEMENT */

void handle_router_response(
                struct mip_unix_msg *res,
                struct mip_daemon_queue *queue,
                struct mip_daemon_fds *fds,
                struct mip_cache *cache)
{
        struct mip_msg *msg;
        struct mip_cache *ptr;

        msg = queue_head_pop(queue->router);

        /* If the router replied with address 255 (broadcast), the query
         * failed. Make an attempt directly with arp instead. */
        if (res->routing.addr == MIP_BROADCAST) { 

                /* Send arp request, then push message into arp-queue, awaiting
                 * processing after reply. */
                send_arp_request(msg->hdr.dst_addr, cache);
                queue_tail_push(queue->arp, msg);

                if (d) printf("<mip_daemon: mip arp sent>\n");
                return;

        /* Else, look up the address the router suggested to route through, and
         * send message there. */
        } else {
                ptr = mip_cache(res->routing.addr, cache);
                if (ptr != NULL)
                        mip_send(fds, msg, ptr);

                /* Implied `else`: if the router tells us to route through an
                 * address not in our cache. This should usually not happen,
                 * but drop the message if it does. */
        }

        free(msg);
}


void process_queue(struct queue *queue,
                   struct mip_daemon_fds *fds,
                   struct mip_cache *cache)
{
        struct queue_entry *ptr;
        struct queue_entry *tmp;
        int rc;

        if (queue_is_empty(queue))
                return;
 
        /* Iterate over queue entries */
        for (ptr = queue->head; ptr != NULL;) {
                tmp = ptr;
                ptr = ptr->next;

                /* Check if we can send a packet. The NULLed out variables are
                 * ok, since we're certain the message is not of type ARP */
                rc = handle_mip_msg(tmp->data, fds, cache, NULL, NULL);
                if (rc != MIP_CACHE_MISS)
                        free(queue_remove(queue, tmp));
        }
}

/* SMALL UTILITIES */

int init_epoll_table(struct mip_daemon_fds *fds)
{
        /* Create epoll table */
        int epollfd = epoll_create1(0);
        if (epollfd == -1) {
                perror("epoll_create");
                return -1;
        }

        /* Add connect socket to epoll table */
        if (epoll_ctl_add(epollfd, EPOLLIN, fds->connect) == -1)
                return -1;

        /* Add raw socket to epoll table */
        if (epoll_ctl_add(epollfd, (EPOLLIN | EPOLLHUP), fds->raw) == -1)
                return -1;

        return epollfd;
}

void close_fds(struct mip_daemon_fds *fds)
{
        if (-1 < fds->epoll)
                close(fds->epoll);
        if (-1 < fds->connect)
                close(fds->connect);
        if (-1 < fds->raw)
                close(fds->raw);
        if (-1 < fds->router)
                close(fds->router);
        if (-1 < fds->ping)
                close(fds->ping);
}

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] [-d] <socket_upper> <MIP address>\n", cmd);
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

        if (argc < 3 || 4 < argc) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}
