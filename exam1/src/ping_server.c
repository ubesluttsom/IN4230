#include <stdio.h>              /* printf */
#include <stdlib.h>             /* exit */
#include <unistd.h>             /* close */
#include <string.h>             /* memset */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>         /* sockets operations */

#include "ping.h"
#include "common.h"

/* Reply to a PING */
static void pong_ping(int sd, struct ping *ping)
{
        /* Change a single letter to make "PING" become "PONG". Lol */
        ping->msg[1] = 'O';

        /* Try to send pong */
        if (send(sd, ping, sizeof(struct ping), 0) < 0) {
                perror("write pong");
                return;
        }

        printf("<ping_server: pong'd>\n");
}

static void server(char *socket_name)
{
        struct epoll_event events[1];
        int sd, epollfd, rc;

        struct ping ping;

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
                rc = epoll_wait(epollfd, events, 1, -1);
                if (rc == -1) {
                        perror("epoll_wait");
                        goto close_sock;

                } else if (events->data.fd == sd) {

                        /* Try to receive a ping */
                        memset(&ping, '\0', sizeof(struct ping));
                        if (recv_ping(events->data.fd, &ping, PING) < 0)
                                goto close_sock;

                        printf("<ping_server: ping from %d> %s\n",
                               ping.addr, ping.msg);

                        /* Reply to ping with a pong */
                        pong_ping(sd, &ping);

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

static void parse_cmd_opts(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "h")) != -1) {
                switch (opt) {
                case 'h':
                        print_usage(argv[0]);
                        exit(EXIT_SUCCESS);
                }
        }

        if (argc != 2) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        server(argv[1]);
}
