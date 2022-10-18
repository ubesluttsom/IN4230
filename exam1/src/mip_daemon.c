#include <errno.h>

#include "mip.h"
#include "mip_arp.h"
#include "mip_daemon.h"
#include "common.h"

/* Defining some return values */
#define SOCK_NOT_INIT -2
#define SOCK_CON_TERM -3
#define MIP_MSG_TTL_Z -4

int pass_to_application(struct mip_msg *msg, int unix_sock)
{
        if (unix_sock < 0)
                /* If the unix socket, for whatever reason, is not initialized.
                 * Perhaps there is no connected application above? */
                return SOCK_NOT_INIT;

        /* Writing message to unix socket, i.e. upper layer application */
        if (write(unix_sock, msg, MIP_MSG_LEN) < 0) {
                if (errno == EPIPE)
                        /* Socket not avalible for writing, likely due to a
                         * connection termination  */
                        return SOCK_CON_TERM;
                else {
                        perror("write");
                        return -1;
                }
        }

        return 0;
}

int handle_ping_packet(struct mip_msg *msg, int unix_sock,
                       struct mip_cache *cache)
{
        struct ether_frame ether_hdr;
        uint8_t mip_self = cache->mip;  /* *Current* MIP-address */
        struct mip_cache *ptr;

        if (msg->hdr.dst_addr == mip_self) {
                /* Ping arrived to recipient (this node) */
                return pass_to_application(msg, unix_sock);
        }

        /* Try to find correct entry in MIP cache */
        for (ptr = cache; ptr != NULL; ptr = ptr->next)
                if (ptr->mip == msg->hdr.dst_addr)
                        break;

        if (ptr == NULL) {
                /* If MIP address is *not* in cache. */
                send_arp_request(msg->hdr.dst_addr, cache);
                return MIP_CACHE_MISS;
        }

        /* Initialize Ethernet header */
        memcpy(&ether_hdr.dst_addr, ptr->mac, ETH_MAC_LEN);
        memcpy(&ether_hdr.src_addr, ptr->addr.sll_addr, ETH_MAC_LEN);
        ether_hdr.eth_proto = htons(ETH_P_MIP);

        /* Update MIP header with sender information (this is unknown to
         * application above, so must be done here). FIXME: ruins multi-hop */
        msg->hdr.src_addr = mip_self;

        /* Send as raw packet */
        send_raw_packet(ptr->raw_socket,
                        (uint8_t *)msg,
                        MIP_MSG_LEN,
                        &ether_hdr,
                        &ptr->addr);
        return 0;
}

int handle_mip_msg(uint8_t * buf,
                   int unix_sock,
                   struct mip_cache *cache,
                   struct ether_frame *frame_hdr,
                   struct sockaddr_ll *so_addr)
{

        struct mip_msg *msg;
        msg = (struct mip_msg *)buf;

        printf("<Handling MIP message>\n");
        print_mip_msg(msg, 0);

        /* Drop the package if TTL har expired, else decrement it */
        if (msg->hdr.ttl == 0)
                return MIP_MSG_TTL_Z;
        else 
                msg->hdr.ttl = msg->hdr.ttl - 1;

        /* Handle the different types of MIP package */
        switch (msg->hdr.sdu_type) {
        case MIP_T_ARP:
                return handle_arp_packet(msg, cache, frame_hdr, so_addr);
        case MIP_T_PING:
                return handle_ping_packet(msg, unix_sock, cache);
        default:
                printf("<error: unknown MIP SDU type>\n");
                return -1;
        }

}

int recv_mip_packet(int fd, uint8_t *buf)
{
        /*
         * TODO: `buf` should be dynamically allocaded!
         */
        int rc;
        struct mip_msg *msg;

        memset(buf, 0, sizeof(struct mip_msg));   /* Null out the buffer */

        /* First, simply PEEK at the message in the buffer. This is to get the
         * actual length of the entire message from the header. */
        rc = recv(fd, buf, sizeof(struct mip_msg), MSG_PEEK);
        if (rc < 0)
                goto return_error;

        /* We assume the message is a MIP message, and we can address it as
         * such. FIXME: Should use propper de-serialization instead of cast! */
        msg = (struct mip_msg *)buf;

        /* Since we know know how long the payload is (the SDU), we can now
         * read the entire message for real */
        rc = recv(fd, msg, MIP_MSG_LEN, 0);
        if (rc < 0)
                goto return_error;

        return rc;

return_error:
        perror("recv_mip_packet");
        return rc;
}

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

void print_usage(char *cmd)
{
        printf("usage: %s [-h] [-d] <socket_upper> <MIP address>\n", cmd);
}

void parse_cmd_opts(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "hd")) != -1) {
                switch (opt) {
                        case 'h':
                                print_usage(argv[0]);
                                exit(EXIT_SUCCESS);

                        case 'd':
                                /* TODO: Do something! */
                                continue;
                }
        }

        if (argc > 4) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

void add_to_queue(struct mip_pkt_queue *queue, struct mip_msg *msg)
{
        for(; queue->next != NULL; queue = queue->next);
        queue->next = calloc(1, sizeof(struct mip_pkt_queue));
        queue->next->msg = calloc(1, MIP_MSG_LEN);
        memcpy(queue->next->msg, msg, MIP_MSG_LEN);
}

void mip_daemon(struct mip_cache *cache,
                int epollfd, int unix_sock, int raw_sock)
{
        int err;   /* Store error return values */
        struct sockaddr_ll so_addr;   /* Address storage */
        struct ether_frame frame_hdr;   /* Ethernet frame storage */

        struct epoll_event events[MAX_EVENTS];   /* Storage of epoll table */

        int app_sd;   /* Temp. socket for connecting applications. FIXME:
                         potentially used uninitialized; only one app at a
                         time; should multiplex? */

        uint8_t buf[BUF_SIZE];   /* Buffer for storing messages. FIXME: should
                                    a pointer and be dynamically allocated */

        struct mip_pkt_queue queue;
        bzero(&queue, sizeof(struct mip_pkt_queue));

        while (1) {

                /* Wait for something to happen on any socket */
                if (epoll_wait(epollfd, events, MAX_EVENTS, -1) == -1) {
                        perror("epoll_wait");
                        goto close_socks;
                }

                if (events->data.fd == unix_sock) {

                        /* Handle incoming application connection */
                        printf("<New application trying to connect ... >\n");
                        app_sd = accept(unix_sock, NULL, NULL);
                        if (app_sd == -1) {
                                perror("accept");
                                continue;
                        }

                        if (epoll_ctl_add(epollfd,  EPOLLIN | EPOLLONESHOT, app_sd) == -1)
                                /* TODO: Fancier error handling? */
                                continue;

                        printf("<Application connected>\n");

                } else if (events->data.fd == raw_sock) {

                        /* Receive data from raw socket */
                        if (recv_raw_packet(raw_sock, buf, BUF_SIZE,
                                                &frame_hdr, &so_addr) < 1) {
                                perror("recv");
                                continue;
                        }

                        if (frame_hdr.eth_proto != htons(ETH_P_MIP))
                                printf("<unknown ethernet protocol>\n");

                        if ((err = handle_mip_msg(buf, app_sd, cache,
                                                &frame_hdr, &so_addr)) < 0) {
                                switch (err) {
                                case SOCK_CON_TERM:
                                        epoll_ctl_del(epollfd, app_sd);
                                        app_sd = -1;
                                        continue;
                                case MIP_MSG_TTL_Z:
                                case SOCK_NOT_INIT:
                                case MIP_CACHE_MISS:
                                        /* TODO: should I handle MIP cache miss
                                         * here? Now it just drops the package.
                                         * Handling MIP cache miss would imply
                                         * asking other nodes, making multiple
                                         * hops */
                                        continue;
                                }
                        }

                } else {
                        /* Application is trying to send data */
                        printf("<Recived data from application>\n");

                        if (recv_mip_packet(events->data.fd, buf) == 0) {
                                /* No bytes read. Connection terminated */
                                epoll_ctl_del(epollfd, events->data.fd);
                                continue;
                        };

                        if ((handle_mip_msg(buf, events->data.fd, cache,
                                            NULL, NULL)) == MIP_CACHE_MISS) {
                                add_to_queue(&queue, (struct mip_msg *)buf);
                        }
                        /* FIXME: Skaper masse rot! */
                        epoll_ctl_mod(epollfd, EPOLLIN | EPOLLONESHOT, events->data.fd);
                        continue;
                }

                /* Try to send packets in the queue. TODO: factor out as
                 * seperate function */

                if (queue.next == NULL)
                        continue;

                struct mip_pkt_queue *ptr_prev = &queue;
                struct mip_pkt_queue *ptr = queue.next;
                for (; ptr != NULL; ptr = ptr->next) {

                        /* Check if we can send a packet */
                        /* TODO: Do something with the NULLed out variables */
                        if ((handle_mip_msg((uint8_t *)ptr->msg, 0, cache, NULL, NULL))
                             != MIP_CACHE_MISS) {

                                /* Remove entry frome packet queue */
                                ptr_prev->next = (ptr->next == NULL)? NULL : ptr->next;
                                free(ptr->msg);
                                free(ptr);
                                ptr = ptr_prev;
                        }

                        ptr_prev = ptr;
                }
        }

close_socks:
        close(epollfd);
        close(unix_sock);
        close(raw_sock);
        exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
        int epollfd;            /* Epoll file descriptor */
        int unix_sock;          /* Unix socket descriptor */
        int raw_sock;           /* Raw socket descriptor */

        uint8_t mip_addr;       /* MIP address of *this* node */
        char *unix_sock_str;   /* Path to unix socket */

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
        struct mip_cache cache; /* List of all network interfaces */
        init_mip_cache(mip_addr, &cache, raw_sock);

        /* Run the main program loop */
        mip_daemon(&cache, epollfd, unix_sock, raw_sock);

        return EXIT_SUCCESS;
}
