#include "ping.h"
#include <stdio.h>              /* standard input/output library functions */
#include <stdlib.h>             /* standard library definitions (macros) */
#include <string.h>             /* string operations (strncpy, memset..) */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <unistd.h>             /* standard symbolic constants and types */

#include "mip.h"
#include "common.h"

static void pong_ping(int sd, struct mip_msg *msg)
{
        uint8_t tmp;

        /* Swap MIP addresses */
        tmp = msg->hdr.dst_addr,
        msg->hdr.dst_addr = msg->hdr.src_addr;
        msg->hdr.src_addr = tmp;

        /* Reset the TTL. Needs to be more than 1, since the `mip_daemon`
           decrements it before sending. And on the other end, drops it if it's
           0. FIXME: remove hardcoded */
        msg->hdr.ttl = 2;  

        /* Change a single letter to make "PING" become "PONG". Lol */
        ((char *)msg->sdu)[1] = 'O';

        /* Write back to socket */
        if (write(sd, msg, MIP_MSG_LEN) <= 0) {
                perror("write pong");
                return;
        }
        printf("<ping_server: pong'd>\n");
}

void server(char *socket_name)
{
        struct epoll_event events[MAX_EVENTS];
        int sd, epollfd, rc;

        struct mip_msg *msg;   /* Storage for recieved MIP messages */

        /* Call the method to create, bind and listen to the server socket */
        sd = init_unix_sock(socket_name, USOCK_CONNECT);

        /* Create the main epoll file descriptor */
        epollfd = epoll_create1(0);
        if (epollfd == -1) {
                perror("epoll_create1");
                goto close_sock;
        }

        /* Add the main listening socket to epoll table */
        rc = epoll_ctl_add(epollfd, EPOLLIN | EPOLLONESHOT, sd);
        if (rc == -1)
                goto close_sock;

        while (1) {
                rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);
                if (rc == -1) {
                        perror("epoll_wait");
                        goto close_sock;

                } else if (events->data.fd == sd) {

                        /* Try to receive a ping */
                        msg = recv_mip_ping(events->data.fd, PING);
                        if (msg == NULL)
                                continue;

                        /* TODO: clean up this output */
                        printf("<ping_server: received ping>\n");
                        print_mip_msg(msg, 0);

                        /* Reply to ping with a pong */
                        pong_ping(sd, msg);

                        /* We don't need the MIP message any more, so ... */
                        free(msg);

                        /* “Rearm” the socket for detection by epoll */
                        epoll_ctl_mod(epollfd, EPOLLIN | EPOLLONESHOT, sd);
                }
        }

        close(sd);
        exit(EXIT_SUCCESS);

 close_sock:
        close(sd);
        exit(EXIT_FAILURE);
}

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] <socket_lower>\n", cmd);
}

int main(int argc, char *argv[])
{
        int opt, hflag = 0;

        while ((opt = getopt(argc, argv, "h")) != -1) {
                if (opt == 'h') {
                        print_usage(argv[0]);
                        hflag = 1;
                }
        }

        if (hflag == 1) {
                return EXIT_SUCCESS;
        } else if (argc == 2) {
                server(argv[1]);
                return EXIT_SUCCESS;
        } else {
                return EXIT_FAILURE;
        }
}
