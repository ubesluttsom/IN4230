#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */

#include "ping.h"
#include "mip.h"
#include "common.h"

struct mip_msg *construct_mip_ping(uint8_t dst_addr, char *buf)
{
        struct mip_msg *msg;
        struct mip_hdr hdr;
        char ping_str[] = "PING:";

        /* Prepare MIP message */
        hdr.dst_addr = dst_addr;
        hdr.src_addr = 0x00;     /* App doesn't know this. FIXME: should it? */
        hdr.sdu_len = (strlen(ping_str) + strlen(buf)) / 4;
        hdr.sdu_type = MIP_T_PING;
        hdr.ttl = 2;     /* Needs to be more than 1, since the `mip_daemon`
                            decrements it before sending. And on the other end,
                            drops it if it's 0. FIXME: remove hardcoded */

        msg = calloc(1, sizeof(struct mip_msg) + hdr.sdu_len*4);
        msg->hdr = hdr;

        /* First, copy over the prefix "PING:" */
        memcpy(msg->sdu, ping_str, strlen(ping_str));

        /* Then , copy over the contents of the buffer, making sure to start at
         * the end of the prefix string */
        memcpy(&((char *)(msg->sdu))[strlen(ping_str)],
               buf, hdr.sdu_len*4 - strlen(ping_str));

        return msg;
}

int client(char* dst_host, char *buf, char *socket_name)
{
        int sd;
        int rc;

        struct epoll_event events[MAX_EVENTS];
        int epollfd;

        struct mip_msg *msg;

        /* Initialize and connect to unix socket */
        sd = init_unix_sock(socket_name, USOCK_CONNECT);

        /* Make the MIP package. Must be free'd later! */
        msg = construct_mip_ping(atoi(dst_host), buf);

        /* Create the main epoll file descriptor */
        epollfd = epoll_create1(0);
        if (epollfd == -1) {
                perror("epoll_create1");
                goto exit_free_close;
        }

        /* Add the main listening socket to epoll table */
        rc = epoll_ctl_add(epollfd, EPOLLIN | EPOLLONESHOT, sd);
        if (rc == -1)
                goto exit_free_close;

        /* Try to send ping */
        printf("<sending ping>\n");
        rc = write(sd, msg, MIP_MSG_LEN);
        if (rc < 0) {
                perror("write");
                goto exit_free_close;
        }

        /* Free this before we loose the reference */
        free(msg);

        /* Wait for pong */
        rc = epoll_wait(epollfd, events, MAX_EVENTS, 1000);

        if (rc == -1) {
                perror("epoll_wait");
                goto exit_close;

        } else if (rc == 0) {
                printf("<ping_client: timed out>\n");
                goto exit_close;

        } else if (events->data.fd == sd) {

                /* Try to receive a pong. Allocates a message */
                msg = recv_mip_ping(events->data.fd, PONG);
                if (msg == NULL)
                        goto exit_close;

                /* TODO: clean up this output */
                printf("<ping_client: received pong>\n");
                print_mip_msg(msg, 0);

        }

        close(sd);
        free(msg);
        return EXIT_SUCCESS;

exit_free_close:
        free(msg);
exit_close:
        close(sd);
        return EXIT_FAILURE;
}

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] <destination_host> <message> <socket_lower>\n",
               cmd);
}

void parse_cmd_opts(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "d")) != -1) {
                switch (opt) {
                        case 'h':
                                print_usage(argv[0]);
                                exit(EXIT_SUCCESS);
                }
        }

        if (argc != 4) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        return client(argv[1], argv[2], argv[3]);
}
