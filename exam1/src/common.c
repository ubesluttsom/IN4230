#include "common.h"
#include "mip.h"
#include "mip_arp.h"

struct mip_msg *construct_mip_ping(uint8_t src_addr, uint8_t dst_addr, uint8_t *buf)
{
        struct mip_msg *msg;
        struct mip_hdr hdr;

        /* Prepare MIP message. Set address and protocol */
        hdr.dst_addr = dst_addr;
        hdr.src_addr = src_addr;
        hdr.sdu_type = MIP_T_PING;

        /* Length is calculated by rounding up the integer division; C
         * truncates the division, so I use this neat math trick: */
        hdr.sdu_len  = (strlen((char *)buf) + (4 - 1)) / 4;

        /* Needs to be more than 1, since the `mip_daemon` decrements it before
         * sending. And on the other end, drops it if it's 0 (before handling)
         * FIXME: remove hardcoded */
        hdr.ttl = 3;

        /* Allocate memory for message, and attach header */
        msg = calloc(1, sizeof(struct mip_msg) + hdr.sdu_len*4);
        msg->hdr = hdr;

        /* Copy over the contents of the buffer */
        memcpy(msg->sdu, buf, hdr.sdu_len*4);

        return msg;
}

void print_mac_addr(uint8_t * addr, size_t len)
{
        size_t i;
        for (i = 0; i < len - 1; i++)
                printf("%02x:", addr[i]);
        printf("%02x", addr[i]);
}

void debug_print(struct ether_frame *eth,
                 struct mip_msg *msg, 
                 struct mip_cache *cache)
{
        /* Sorry for this monster. If the SDU is to long, we need to break
         * lines and calculate padding to fit it into a neat box. (I spent way
         * to long on getting this right ...) */

        int sdu_len = strnlen((char *)msg->sdu, msg->hdr.sdu_len*4);
        int line_len = (sdu_len > 62) ? 62 : sdu_len;

        int hdr_pad_len = (line_len < 26) ? 1 : line_len - 26;
        int sdu_pad_len = (line_len > 26) ? 1 : 28 - line_len;

        char hdr_padding[hdr_pad_len];
        char sdu_padding[sdu_pad_len];
        char dashes[hdr_pad_len];

        memset(hdr_padding, ' ', sizeof(hdr_padding));
        memset(sdu_padding, ' ', sizeof(sdu_padding));
        memset(dashes, '-', sizeof(dashes));
        hdr_padding[sizeof(hdr_padding) - 1] = '\0';
        sdu_padding[sizeof(sdu_padding) - 1] = '\0';
        dashes[sizeof(dashes) - 1] = '\0';

        printf("+- mip_cache -----------------%s+\n", dashes);
        for (cache = cache->next; cache != NULL; cache = cache->next) {
                if (cache->mip == MIP_BROADCAST)
                        continue;
                printf("| % 3d:      ", cache->mip);
                print_mac_addr(cache->mac, 6);
                printf(" |\n");
        }

        printf("+- eth_hdr -------------------%s+\n", dashes);
        printf("| dst_addr: ");
        print_mac_addr(eth->dst_addr, 6);
        printf(" |\n");
        printf("| src_addr: ");
        print_mac_addr(eth->src_addr, 6);
        printf(" |\n");
        printf("+- mip_hdr -------------------%s+\n", dashes);
        printf("| dst_addr: % 17d%s |\n", msg->hdr.dst_addr, hdr_padding);
        printf("| src_addr: % 17d%s |\n", msg->hdr.src_addr, hdr_padding);
        printf("| ttl:      % 17d%s |\n", msg->hdr.ttl, hdr_padding);
        printf("| sdu_len:  % 17d%s |\n", msg->hdr.sdu_len, hdr_padding);
        printf("| sdu_type: % 17d%s |\n", msg->hdr.sdu_type, hdr_padding);
        printf("+- sdu -----------------------%s+\n", dashes);

        if (msg->hdr.sdu_type == MIP_T_ARP) {
                struct mip_arp *arp = (struct mip_arp *)msg->sdu;
                printf("| type:     % 17d%s |\n", arp->type, hdr_padding);
                printf("| addr:     % 17d%s |\n", arp->addr, hdr_padding);
                printf("+-----------------------------%s+\n", dashes);
                return;
        }

        /* else MIP_T_PING is assumed */

        if (sdu_len <= 62) {
                printf("| %s%s |\n", (char *)msg->sdu, sdu_padding);
                printf("+-----------------------------%s+\n", dashes);
                return;
        }

        printf("| ");
        for (int i = 0; i < sdu_len; i++) {
                printf("%c", ((char *)msg->sdu)[i]);
                if ((i+1) % 62 == 0)
                        printf(" |\n| ");
        }
        for (int i = line_len - (sdu_len % 62); i > 0; i--)
                printf(" ");
        printf(" |\n");
        printf("+-----------------------------%s+\n", dashes);
}

void print_mip_msg(struct mip_msg *msg, int hex)
{
        int sdu_len = strnlen((char *)msg->sdu, msg->hdr.sdu_len*4);
        int line_len = (sdu_len > 62) ? 62 : sdu_len;

        int hdr_pad_len = (line_len < 14) ? 1 : line_len - 14;
        int sdu_pad_len = (line_len > 14) ? 1 : 16 - line_len;

        char hdr_padding[hdr_pad_len];
        char sdu_padding[sdu_pad_len];
        char dashes[hdr_pad_len];

        memset(hdr_padding, ' ', sizeof(hdr_padding));
        memset(sdu_padding, ' ', sizeof(sdu_padding));
        memset(dashes, '-', sizeof(dashes));
        hdr_padding[sizeof(hdr_padding) - 1] = '\0';
        sdu_padding[sizeof(sdu_padding) - 1] = '\0';
        dashes[sizeof(dashes) - 1] = '\0';

        printf("+- hdr -----%s------+\n", dashes);
        printf("| dst_addr: % 5d%s |\n", msg->hdr.dst_addr, hdr_padding);
        printf("| src_addr: % 5d%s |\n", msg->hdr.src_addr, hdr_padding);
        printf("| ttl:      % 5d%s |\n", msg->hdr.ttl, hdr_padding);
        printf("| sdu_len:  % 5d%s |\n", msg->hdr.sdu_len, hdr_padding);
        printf("| sdu_type: % 5d%s |\n", msg->hdr.sdu_type, hdr_padding);
        printf("+- sdu -----%s------+\n", dashes);

        if (msg->hdr.sdu_type == MIP_T_ARP) {
                struct mip_arp *arp = (struct mip_arp *)msg->sdu;
                printf("| type:     % 5d%s |\n", arp->type, hdr_padding);
                printf("| addr:     % 5d%s |\n", arp->addr, hdr_padding);
                printf("+-----------%s------+\n", dashes);
                return;
        }

        /* else MIP_T_PING is assumed */

        if (sdu_len <= 62) {
                printf("| %s%s |\n", (char *)msg->sdu, sdu_padding);
                printf("+-----------%s------+\n", dashes);
                return;
        }

        /* Sorry for this monster. If the SDU is to long, we need to break
         * lines to fit it into a neat box. (I spent way to long on getting
         * this right ...) */
        printf("| ");
        for (int i = 0; i < sdu_len; i++) {
                if (hex) {
                        printf("%02x", ((uint8_t *)msg->sdu)[++i]);
                } else {
                        printf("%c", ((char *)msg->sdu)[i]);
                }
                if ((i+1) % 62 == 0)
                        printf(" |\n| ");
        }
        for (int i = line_len - (sdu_len % 62); i > 0; i--)
                printf(" ");
        printf(" |\n");
        printf("+-----------%s------+\n", dashes);
}

int send_raw_packet(int sd, uint8_t *buf, size_t len,
                    struct ether_frame *frame_hdr,
                    struct sockaddr_ll *so_addr)
{
        int rc;
        struct iovec msgvec[2];
        struct msghdr msghdr = {0};

        /* Point to frame header */
        msgvec[0].iov_base = frame_hdr;
        msgvec[0].iov_len = sizeof(struct ether_frame);
        /* Point to frame payload */
        msgvec[1].iov_base = buf;
        msgvec[1].iov_len = len;

        /* Fill out message metadata struct */
        msghdr.msg_name = so_addr;
        msghdr.msg_namelen = sizeof(struct sockaddr_ll);
        msghdr.msg_iovlen = 2;
        msghdr.msg_iov = msgvec;

        rc = sendmsg(sd, &msghdr, 0);
        if (rc < 0)
                perror("sendmsg");

        return rc;
}

struct mip_msg *recv_raw_packet(int sd,
                                struct ether_frame *frame_hdr,
                                struct sockaddr_ll *so_addr)
{
        int rc;
        struct iovec msgvec[2];
        struct msghdr msghdr;

        memset(&msghdr, '\0', sizeof(msghdr));

        /* Make room for a MIP message header */
        struct mip_msg *msg = calloc(1, sizeof(struct mip_msg));

        /* Point to frame header */
        msgvec[0].iov_base = frame_hdr;
        msgvec[0].iov_len = sizeof(struct ether_frame);

        /* Point to frame payload */
        msgvec[1].iov_base = msg;
        msgvec[1].iov_len = sizeof(struct mip_msg);

        /* Fill out message metadata struct */
        msghdr.msg_name = so_addr;
        msghdr.msg_namelen = sizeof(struct sockaddr_ll);
        msghdr.msg_iovlen = 2;
        msghdr.msg_iov = msgvec;

        /* Take a peek at the MIP message header */
        rc = recvmsg(sd, &msghdr, MSG_PEEK);
        if (rc == -1)
                goto recv_error;

        /* Realloc just the space we need to retrieve the entire message.
         * FIXME: I seem to occationally get a fail here. If the peeked at
         * message is corrupted, it might be giving invalid sizes in
         * `msg->hdr.sdu_len`, or something. */
        msg = realloc(msg, MIP_MSG_LEN);

        msgvec[1].iov_len = MIP_MSG_LEN;

        rc = recvmsg(sd, &msghdr, 0);
        if (rc == -1)
                goto recv_error;

        return msg;

recv_error:
        perror("recv_raw_packet");
        free(msg);
        return NULL;
}

int epoll_ctl_add(int epollfd, uint32_t events, int fd)
{
        struct epoll_event ev;

        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                perror("epoll_ctl_add");
                return -1;
        }

        return 0;
}

int epoll_ctl_mod(int epollfd, uint32_t events, int fd)
{
        struct epoll_event ev;

        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                return -1;

        return 0;
}

int epoll_ctl_del(int epollfd, int fd)
{
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                if (errno != ENOENT) {
                        perror("epoll_ctl_del");
                        close(fd);
                        return -1;
                }
        }
        close(fd);
        return 0;
}

int init_unix_sock(char *socket_name, uint8_t flags)
{
        /*
         * TODO: Review this function.
         */
        struct sockaddr_un addr;
        int sd;

        sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (sd == -1) {
                perror("socket()");
                exit(EXIT_FAILURE);
        }

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_name, sizeof(addr.sun_path) - 1);

        if (flags & USOCK_BIND) {
                if (bind(sd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
                        perror("bind");
                        goto close_sock;
                }
        }

        if (flags & USOCK_LISTEN) {
                if (listen(sd, MAX_CONNS) == -1) {
                        perror("listen");
                        goto close_sock;
                }
        }

        if (flags & USOCK_CONNECT) {
                if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
                        perror("connect");
                        goto close_sock;
                }
        }

        return sd;

close_sock:
        close(sd);
        exit(EXIT_FAILURE);
}

int init_raw_sock(void)
{
        int raw_sock;

        raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_MIP));
        if (raw_sock == -1) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        return raw_sock;
}
