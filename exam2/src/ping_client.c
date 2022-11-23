#include <stdlib.h>             /* exit */
#include <unistd.h>             /* close */
#include <stdio.h>              /* printf */
#include <string.h>             /* memcpy */
#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <sys/epoll.h>          /* epoll */
#include <time.h>               /* timer */

#include "mip_unix.h"
#include "common.h"

/* TTL is an optional commandline argument, but default the value below.
   (Using a global assignment here is fine, since it only changed once at
   program start.) */
static int ttl = TTL_DEFAULT;

static void client(char *dst_host, char *buf, char *socket_name)
{
        int rc, sd, epollfd;
        struct epoll_event event[1];    /* Only need a single event */

        struct mip_unix_msg ping;

        clock_t time;
        float usec;

        /* Connect to MIP daemon on layer below */
        sd = mip_connect(socket_name, MIP_T_PING);
        if (sd < 0)
                exit(EXIT_FAILURE);

        /* Initialize ping structure */
        memset(&ping, '\0', sizeof(struct mip_unix_msg));
        ping.addr = atoi(dst_host);
        ping.ttl = ttl;
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
                goto close_all;

        /* Try to send ping */
        printf("<ping_client: sending ping>\n");
        rc = send_ping(sd, &ping);
        if (rc < 0)
                goto close_all;

        /* Zero out buffer before recieving anything */
        memset(&ping, '\0', BUF_SIZE);

        /* Start timer */
        time = clock();

        /* Wait for pong */
        rc = epoll_wait(epollfd, event, 1, 1000);

        if (rc == -1) {
                perror("epoll_wait");
                goto close_all;

        } else if (rc == 0) {
                printf("<ping_client: timeout>\n");
                goto close_all;

        } else if (event->data.fd == sd) {
                if (recv_ping(event->data.fd, &ping, PONG) < 0) {
                        printf("<ping_client: unknown response>\n");
                        goto close_all;
                }
        }

        /* Calculate time difference. Convert to microseconds */
        time = clock() - time;
        usec = ((float)time / CLOCKS_PER_SEC) * 1000000;

        printf("<ping_client: pong from %d in %.0fus> %s\n",
               ping.addr, usec, ping.msg);

        close(sd);
        exit(EXIT_SUCCESS);

 close_all:
        close(epollfd);
 close_sock:
        close(sd);
        exit(EXIT_FAILURE);
}

static void print_usage(char *cmd)
{
        printf("usage: %s ", cmd);
        printf("[-h] [-t ttl] ");
        printf("<destination_host> <message> <socket_lower>\n");
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

        if (argc < 4 || argc > 6) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[])
{
        parse_cmd_opts(argc, argv);
        client(argv[argc - 3], argv[argc - 2], argv[argc - 1]);
}
