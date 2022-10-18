#include "common.h"
#include "mip.h"
#include "mip_arp.h"

#include <unistd.h>

struct mip_msg *recv_mip_ping(int sd, uint8_t type)
{
        struct mip_msg *msg = (struct mip_msg *)malloc(sizeof(struct mip_msg));

        /* Peek at the message in the file descriptor. */
        if (recv(sd, msg, sizeof(struct mip_msg), MSG_PEEK) < 0)
                goto recv_error;

        /* Resize the message buffer */
        msg = (struct mip_msg *)realloc(msg, MIP_MSG_LEN);

        /* Peek at it *again*, but this time read the entire thing */
        if (recv(sd, msg, MIP_MSG_LEN, MSG_PEEK) < 0)
                goto recv_error;

        if (type & PING) {
                /* If the message is a PING, read it for real (remove from socket) */
                if (strncmp((char *)msg->sdu, "PING:", 5) == 0)
                        if (recv(sd, msg, MIP_MSG_LEN, 0) < 0)
                                goto recv_error;

        } else if (type & PONG) {
                /* If the message is a PONG, read it for real (remove from socket) */
                if (strncmp((char *)msg->sdu, "PONG:", 5) == 0)
                        if (recv(sd, msg, MIP_MSG_LEN, 0) < 0)
                                goto recv_error;
        } else {
                goto exit_free;
        }

        return msg;

recv_error:
        perror("recv_ping");
exit_free:
        free(msg);
        return NULL;
}

void print_mac_addr(uint8_t * addr, size_t len)
{
        size_t i;
        for (i = 0; i < len - 1; i++)
                printf("%02x:", addr[i]);
        printf("%02x", addr[i]);
}

void print_mip_msg(struct mip_msg *msg, int hex)
{
        int sdu_len = msg->hdr.sdu_len*4;
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
        char c;
        for (int i = 0; i < sdu_len; i++) {
                if (hex) {
                        printf("%02x", ((uint8_t *)msg->sdu)[++i]);
                } else {
                        c = ((char *)msg->sdu)[i];
                        c = (c == '\0')? ' ' : c;
                        printf("%c", c);
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
        struct msghdr *msg;
        struct iovec msgvec[2];

        if (frame_hdr == NULL) {        // FIXME: Dumb temporary workaround
                struct ether_frame tmp;
                frame_hdr = &tmp;

                /* Fill in Ethernet header */
                uint8_t dst_addr[] = ETH_BROADCAST;
                memcpy(frame_hdr->dst_addr, dst_addr, ETH_MAC_LEN);
                memcpy(frame_hdr->src_addr, so_addr->sll_addr, ETH_MAC_LEN);
                frame_hdr->eth_proto = htons(ETH_P_MIP);
        }

        /* Point to frame header */
        msgvec[0].iov_base = frame_hdr;
        msgvec[0].iov_len = sizeof(struct ether_frame);
        /* Point to frame payload */
        msgvec[1].iov_base = buf;
        msgvec[1].iov_len = len;

        /* Allocate a zeroed-out message info struct */
        msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

        /* Fill out message metadata struct */
        msg->msg_name = so_addr;
        msg->msg_namelen = sizeof(struct sockaddr_ll);
        msg->msg_iovlen = 2;
        msg->msg_iov = msgvec;

        /* TODO: review debug prints */
        printf("<Sending MIP packet to ");
        print_mac_addr(frame_hdr->dst_addr, 6);
        printf(" from ");
        print_mac_addr(frame_hdr->src_addr, 6);
        printf(">\n");

        rc = sendmsg(sd, msg, 0);
        if (rc == -1)
                perror("sendmsg");

        free(msg);
        return rc;
}

int recv_raw_packet(int sd, uint8_t * buf, size_t len,
                    struct ether_frame *frame_hdr,
                    struct sockaddr_ll *so_addr)
{
        struct msghdr msg;
        struct iovec msgvec[2];

        if (frame_hdr == NULL) {        /* FIXME: Dumb temporary workaround */
                struct ether_frame tmp;
                frame_hdr = &tmp;
        }

        /* Point to frame header */
        msgvec[0].iov_base = frame_hdr;
        msgvec[0].iov_len = sizeof(struct ether_frame);

        /* Point to frame payload */
        msgvec[1].iov_base = buf;
        msgvec[1].iov_len = len;

        /* Fill out message metadata struct */
        msg.msg_name = so_addr;
        msg.msg_namelen = sizeof(struct sockaddr_ll);
        msg.msg_iovlen = 2;
        msg.msg_iov = msgvec;

        if (recvmsg(sd, &msg, 0) == -1) {
                perror("sendmsg");
                return -1;
        }

        /* TODO: review debug prints */
        printf("<");
        print_mac_addr(frame_hdr->src_addr, 6);
        printf(">: 0x");
        for (unsigned int i = 0; i < len; i++) {
                if (buf[i] != 0x00) {
                        printf("%x", buf[i]);
                }
        }
        printf("\n");

        return 1;
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
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                perror("epoll_ctl_mod");
                return -1;
        }

        return 0;
}

int epoll_ctl_del(int epollfd, int fd)
{
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                perror("epoll_ctl_del");
                return -1;
        }

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
