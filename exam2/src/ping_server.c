#include <stdio.h>              /* printf */
#include <stdlib.h>             /* exit */
#include <unistd.h>             /* close */
#include <string.h>             /* memset */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>         /* sockets operations */

#include "mip_unix.h"
#include "common.h"

/* TTL is an optional commandline argument, but default the value below.
   (Using a global assignment here is fine, since it only changed once at
   program start.) */
static int ttl = TTL_DEFAULT;

/* Reply to a PING */
static int pong_ping(int sd, struct mip_unix_msg *ping)
{
        int rc;

        /* Change a single letter to make "PING" become "PONG". Lol */
        ping->msg[1] = 'O';

        /* Reset TTL */
        ping->ttl = ttl;

        /* Try to send pong */
        rc = send_data(sd, ping, sizeof(struct mip_unix_msg));
        if (rc < 0)
                return rc;

        printf("<ping_server: pong'd>\n");
        return rc;
}

static void server(char *socket_name)
{
        struct epoll_event events[1];
        int sd, epollfd, rc;
        struct mip_unix_msg ping;

        /* Connect to MIP daemon on layer below */
        sd = mip_connect(socket_name, MIP_T_PING);
        if (sd < 0)
                exit(EXIT_FAILURE);

        /* Create the main epoll file descriptor */
        epollfd = epoll_create1(0);
        if (epollfd == -1) {
                perror("epoll_create1");
                goto close_sock;
        }

        /* Add the main listening socket to epoll table */
        rc = epoll_ctl_add(epollfd, EPOLLIN | EPOLLONESHOT, sd);
        if (rc == -1)
                goto close_all;

        while (1) {
                rc = epoll_wait(epollfd, events, 1, -1);
                if (rc == -1) {
                        perror("epoll_wait");
                        goto close_all;

                } else if (events->data.fd == sd) {

                        /* Try to receive a ping */
                        memset(&ping, '\0', sizeof(struct mip_unix_msg));
                        rc = recv_ping(events->data.fd, &ping, PING);
                        if (rc == SOCK_CON_TERM)
                                /* Only exit if connection is terminated. */
                                goto close_all;
                        else if (rc < 0)
                                /* Discard other data */
                                continue;

                        printf("<ping_server: ping from %d> %s\n",
                               ping.addr, ping.msg);

                        /* Reply to ping with a pong */
                        rc = pong_ping(sd, &ping);
                        if (rc < 0)
                                goto close_all;

                        /* “Rearm” the socket for detection by epoll */
                        epoll_ctl_mod(epollfd, EPOLLIN | EPOLLONESHOT, sd);
                }
        }

        close(sd);
        exit(EXIT_SUCCESS);

 close_all:
        close(epollfd);
 close_sock:
        if (rc == SOCK_CON_TERM)
                printf("<ping_server: connection terminated>\n");
        close(sd);
        exit(EXIT_FAILURE);
}

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] [-t ttl] <socket_lower>\n", cmd);
}

static void parse_cmd_opts(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "ht:")) != -1) {
                switch (opt) {
                case 't':
                        ttl = atoi(optarg);
                        break;
                case 'h':
                        print_usage(argv[0]);
                        exit(EXIT_SUCCESS);
                default:
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                }
        }

        if (argc < 1 || argc > 3) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        server(argv[argc - 1]);
}
