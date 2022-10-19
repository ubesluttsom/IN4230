#include <stdlib.h>             /* exit */
#include <unistd.h>             /* close */
#include <stdio.h>              /* printf */
#include <string.h>             /* memcpy */
#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <sys/epoll.h>          /* epoll */
#include <time.h>               /* timer */

#include "ping.h"
#include "common.h"

static void client(char* dst_host, char *buf, char *socket_name)
{
        int sd;
        int rc;

        struct epoll_event event[1];   /* Only need a single event */
        int epollfd;

        struct ping ping;

        clock_t time;
        float usec;

        /* Initialize and connect to unix socket */
        sd = init_unix_sock(socket_name, USOCK_CONNECT);

        /* Initialize ping structure */
        memset(&ping, '\0', sizeof(struct ping));
        ping.addr = atoi(dst_host);
        memcpy(&ping.msg[0], &"PING:", strlen("PING:"));
        memcpy(&ping.msg[5], buf, strnlen(buf, BUF_SIZE));

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

        /* Try to send ping */
        printf("<ping_client: sending ping>\n");
        rc = send_ping(sd, &ping);
        if (rc < 0)
                goto close_sock;

        /* Zero out buffer before recieving anything */
        memset(&ping, '\0', BUF_SIZE);

        /* Start timer */
        time = clock();

        /* Wait for pong */
        rc = epoll_wait(epollfd, event, 1, 1000);

        if (rc == -1) {
                perror("epoll_wait");
                goto close_sock;

        } else if (rc == 0) {
                printf("<ping_client: timeout>\n");
                goto close_sock;

        } else if (event->data.fd == sd) {
                if (recv_ping(event->data.fd, &ping, PONG) < 0) {
                        printf("<ping_client: unknown response>\n");
                        goto close_sock;
                }
        }

        /* Calculate time difference. Convert to microseconds */
        time = clock() - time;
        usec = ((float) time / CLOCKS_PER_SEC) * 1000000;

        /* TODO: clean up this output */
        printf("<ping_client: pong from %d in %.0fus> %s\n",
                        ping.addr, usec, ping.msg);

        close(sd);
        exit(EXIT_SUCCESS);

close_sock:
        close(sd);
        exit(EXIT_FAILURE);
}

static void print_usage(char *cmd)
{
        printf("usage: %s [-h] <destination_host> <message> <socket_lower>\n",
               cmd);
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

        if (argc != 4) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        client(argv[1], argv[2], argv[3]);
}
