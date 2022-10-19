#include "mip_daemon.h"

/* Debug flag. Using global, since it is set once, then never changed. */
static int d = 0;

/* MAIN */

int main(int argc, char *argv[])
{
        int epollfd;            /* Epoll file descriptor */
        int unix_sock;          /* Unix socket descriptor */
        int raw_sock;           /* Raw socket descriptor */

        uint8_t mip_addr;       /* MIP address of *this* node */
        char *unix_sock_str;    /* Path to unix socket */

        struct mip_cache cache; /* List of all network interfaces */

        /* Parse command line options and arguments */
        parse_cmd_opts(argc, argv);
        mip_addr = atoi(argv[argc - 1]);
        unix_sock_str = argv[argc - 2];

        /* Unlink the socket so that we can reuse the program. This is bad
           hack! Better solution with a lock file, or signal handling. Check
           https://gavv.github.io/articles/unix-socket-reuse. FIXME! */
        unlink(unix_sock_str);

        /* Initialize sockets */
        unix_sock = init_unix_sock(unix_sock_str, USOCK_BIND | USOCK_LISTEN);
        raw_sock = init_raw_sock();
        epollfd = init_epoll_table(unix_sock, raw_sock);

        /* Initialize the MIP-ARP cache */
        init_mip_cache(mip_addr, &cache, raw_sock);

        /* Run the main program loop */
        mip_daemon(&cache, epollfd, unix_sock, raw_sock);

        return EXIT_SUCCESS;
}

void mip_daemon(struct mip_cache *cache,
                int epollfd, int unix_sock, int raw_sock)
{
        struct epoll_event events[MAX_EVENTS];  /* Storage of epoll table */

        int app_sd;             /* Temp. socket for connecting applications.
                                   FIXME: only one app at a time; should
                                   multiplex? */

        /* Queue for storing packages waiting to be processed. They get added
           after MIP-ARP cache misses */
        struct mip_pkt_queue queue;
        memset(&queue, '\0', sizeof(struct mip_pkt_queue));

        app_sd = -1;

        /* Main program loop */
        while (1) {

                /* Wait for something to happen on any socket */
                if (epoll_wait(epollfd, events, MAX_EVENTS, -1) == -1) {
                        perror("epoll_wait");
                        goto close_socks;
                }

                /* Handle incoming application connection */
                if (events->data.fd == unix_sock) {
                        app_sd = handle_new_connection(epollfd, unix_sock);

                        /* Receive data from raw socket */
                } else if (events->data.fd == raw_sock) {
                        if (handle_raw_sock(epollfd, raw_sock,
                                            &app_sd, cache) < 0)
                                continue;
                        /* Try to send packets in the queue after recieving raw
                           packets, since they might have updated the cache */
                        process_queue(&queue, cache);

                        /* Application is trying to send data */
                } else {
                        handle_application(epollfd, &app_sd, events, cache,
                                           &queue);
                }
        }

 close_socks:
        close(epollfd);
        close(unix_sock);
        close(raw_sock);
        exit(EXIT_FAILURE);
}

/* PROGRAM FLOW */

void handle_application(int epollfd,
                        int *app_sd,
                        struct epoll_event events[],
                        struct mip_cache *cache, struct mip_pkt_queue *queue)
{
        int rc;
        struct mip_msg *msg;
        struct ping ping;

        /* Try to read from the application */
        if ((rc = recv_ping(events->data.fd, &ping, PING | PONG)) == 0)
                /* No bytes read. Connection terminated */
                goto epoll_del;

        if (rc < 0)
                return;

        /* Use buffer received from app to make a MIP packet */
        msg = construct_mip_ping(cache->mip, ping.addr, (uint8_t *) ping.msg);

        /* Process MIP packet */
        if ((handle_mip_msg(msg, events->data.fd, cache, NULL, NULL))
            == MIP_CACHE_MISS)
                /* If recipient MAC is unknown, add to queue, to be process
                   after replies from ARP. */
                add_to_queue(queue, msg);

        /* Free MIP message */
        free(msg);

        /* “Rearm” epoll file descriptor, delete it if this fails */
        if (epoll_ctl_mod(epollfd, EPOLLIN | EPOLLONESHOT, events->data.fd) < 0)
                goto epoll_del;
        else
                return;

 epoll_del:
        epoll_ctl_del(epollfd, events->data.fd);
        *app_sd = -1;
}

int handle_raw_sock(int epollfd, int raw_sock, int *app_sd,
                    struct mip_cache *cache)
{

        int err;                /* Store error return values */
        struct sockaddr_ll so_addr;     /* Address storage */
        struct ether_frame frame_hdr;   /* Ethernet frame storage */
        struct mip_msg *msg;    /* Temporary MIP message pointer */

        msg = recv_raw_packet(raw_sock, &frame_hdr, &so_addr);
        if (msg == NULL)
                return -1;

        if (frame_hdr.eth_proto != htons(ETH_P_MIP)) {
                if (d)
                        printf("<mip_daemon: unknown ethernet protocol>\n");
                return -1;
        }

        /* Print recvieved MIP message (cache is printed *before* update) */
        if (d)
                printf("<mip_daemon: mip received>\n");
        if (d)
                debug_print(&frame_hdr, msg, cache);

        err = handle_mip_msg(msg, *app_sd, cache, &frame_hdr, &so_addr);

        free(msg);

        if (err < 0) {
                switch (err) {
                case SOCK_CON_TERM:
                        /* If the socket connection is terminated, delete it
                           from the epoll list */
                        epoll_ctl_del(epollfd, *app_sd);
                        *app_sd = -1;
                        break;
                case MIP_MSG_TTL_Z:
                case SOCK_NOT_INIT:
                case MIP_CACHE_MISS:
                        /* TODO: should I handle MIP cache miss here? Now it
                           just drops the package. Handling MIP cache miss
                           would imply asking other nodes, making multiple hops 
                         */
                        break;
                }
                return -1;
        }
        return 0;
}

int handle_new_connection(int epollfd, int unix_sock)
{
        int app_sd;

        app_sd = accept(unix_sock, NULL, NULL);
        if (app_sd < 0) {
                perror("accept");
                return -1;
        }

        if (epoll_ctl_add(epollfd, EPOLLIN | EPOLLONESHOT, app_sd) < 0) {
                /* TODO: Fancier error handling? */
                close(app_sd);
                return -1;
        }

        if (d)
                printf("<mip_daemon: application connected>\n");

        return app_sd;
}

/* DEALING WITH MIP MESSAGES */

int handle_mip_msg(struct mip_msg *msg,
                   int unix_sock,
                   struct mip_cache *cache,
                   struct ether_frame *frame_hdr, struct sockaddr_ll *so_addr)
{
        /* Drop the package if TTL har expired */
        if (msg->hdr.ttl == 0)
                return MIP_MSG_TTL_Z;

        /* Handle the different types of MIP package */
        switch (msg->hdr.sdu_type) {
        case MIP_T_ARP:
                return handle_arp_packet(msg, cache, frame_hdr, so_addr);
        case MIP_T_PING:
                return handle_ping_packet(msg, unix_sock, cache);
        default:
                if (d)
                        printf("<mip_daemon: error: unknown MIP SDU type>\n");
                return -1;
        }

}

int handle_ping_packet(struct mip_msg *msg, int unix_sock,
                       struct mip_cache *cache)
{
        struct ether_frame ether_hdr;
        uint8_t mip_self = cache->mip;  /* *Current* MIP-address */
        struct mip_cache *ptr;
        struct ping ping;

        if (msg->hdr.dst_addr == mip_self) {
                /* Ping arrived to recipient (this node); pass it to app */
                memset(&ping, '\0', sizeof(struct ping));
                memcpy(&ping.addr, &msg->hdr.src_addr, sizeof(uint8_t));

                /* Make sure no buffer is overrun */
                if (msg->hdr.sdu_len * 4 > sizeof(ping.msg))
                        memcpy(&ping.msg, msg->sdu, sizeof(ping.msg));
                else
                        memcpy(&ping.msg, msg->sdu, msg->hdr.sdu_len * 4);

                /* Try to send it */
                return send_ping(unix_sock, &ping);
        }

        /* Try to find correct entry in MIP cache */
        for (ptr = cache; ptr != NULL; ptr = ptr->next)
                if (ptr->mip == msg->hdr.dst_addr)
                        break;

        if (ptr == NULL) {
                /* If MIP address is *not* in cache. */
                msg = send_arp_request(msg->hdr.dst_addr, cache);

                /* Print sent MIP message */
                if (d)
                        printf("<mip_daemon: mip arp sent>\n");
                if (d)
                        debug_print(&ether_hdr, msg, cache);

                free(msg);
                return MIP_CACHE_MISS;
        }

        /* Initialize Ethernet header */
        memcpy(&ether_hdr.dst_addr, ptr->mac, ETH_MAC_LEN);
        memcpy(&ether_hdr.src_addr, ptr->addr.sll_addr, ETH_MAC_LEN);
        ether_hdr.eth_proto = htons(ETH_P_MIP);

        /* Decrement TTL before sending */
        msg->hdr.ttl = msg->hdr.ttl - 1;

        /* Send as raw packet */
        send_raw_packet(ptr->raw_socket,
                        (uint8_t *) msg, MIP_MSG_LEN, &ether_hdr, &ptr->addr);

        /* Print sent MIP message */
        if (d)
                printf("<mip_daemon: ping sent>\n");
        if (d)
                debug_print(&ether_hdr, msg, cache);

        return 0;
}

struct mip_msg *construct_mip_ping(uint8_t src_addr, uint8_t dst_addr,
                                   uint8_t * buf)
{
        struct mip_msg *msg;
        struct mip_hdr hdr;

        /* Prepare MIP message. Set address and protocol */
        hdr.dst_addr = dst_addr;
        hdr.src_addr = src_addr;
        hdr.sdu_type = MIP_T_PING;

        /* Length is calculated by rounding up the integer division; C
           truncates the division, so I use this neat math trick: */
        hdr.sdu_len = (strlen((char *)buf) + (4 - 1)) / 4;

        /* Needs to be more than 1, since the `mip_daemon` decrements it before
           sending. And on the other end, drops it if it's 0 (before handling)
           FIXME: remove hardcoded */
        hdr.ttl = 3;

        /* Allocate memory for message, and attach header */
        msg = calloc(1, sizeof(struct mip_msg) + hdr.sdu_len * 4);
        msg->hdr = hdr;

        /* Copy over the contents of the buffer */
        memcpy(msg->sdu, buf, hdr.sdu_len * 4);

        return msg;
}

/* QUEUE MANAGEMENT */

void add_to_queue(struct mip_pkt_queue *queue, struct mip_msg *msg)
{
        for (; queue->next != NULL; queue = queue->next) ;
        queue->next = calloc(1, sizeof(struct mip_pkt_queue));
        queue->next->msg = calloc(1, MIP_MSG_LEN);
        memcpy(queue->next->msg, msg, MIP_MSG_LEN);
}

void process_queue(struct mip_pkt_queue *queue, struct mip_cache *cache)
{
        struct mip_pkt_queue *ptr_prev;
        struct mip_pkt_queue *ptr;

        if (queue->next == NULL)
                return;

        ptr_prev = queue;
        ptr = queue->next;

        for (; ptr != NULL; ptr = ptr->next) {

                /* Check if we can send a packet */
                /* TODO: Do something with the NULLed out variables */
                if ((handle_mip_msg(ptr->msg, 0, cache, NULL, NULL))
                    != MIP_CACHE_MISS) {

                        /* Remove entry frome packet queue */
                        ptr_prev->next = (ptr->next == NULL) ? NULL : ptr->next;
                        free(ptr->msg);
                        free(ptr);
                        ptr = ptr_prev;
                }

                ptr_prev = ptr;
        }
}

/* SMALL UTILITIES */

int init_epoll_table(int unix_sock, int raw_sock)
{
        /* Create epoll table */
        int epollfd = epoll_create1(0);
        if (epollfd == -1) {
                perror("epoll_create");
                goto close_socks;
        }

        /* Add unix socket to epoll table */
        if (epoll_ctl_add(epollfd, EPOLLIN, unix_sock) == -1)
                goto close_socks;

        /* Add raw socket to epoll table */
        if (epoll_ctl_add(epollfd, (EPOLLIN | EPOLLHUP), raw_sock) == -1)
                goto close_socks;

        return epollfd;

 close_socks:
        close(unix_sock);
        close(raw_sock);
        exit(EXIT_FAILURE);
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

        if (argc > 4) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}
