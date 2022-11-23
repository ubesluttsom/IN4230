#include "mip_unix.h"

int init_unix_sock(char *sock_path, uint8_t flags)
{
        /* 
         * TODO: Review this function.
         */
        struct sockaddr_un addr;
        int sd;

        sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (sd == -1) {
                perror("socket()");
                return -1;
        }

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

        if (flags & USOCK_BIND) {
                if (bind(sd, (const struct sockaddr *)&addr, sizeof(addr)) ==
                    -1) {
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
        return -1;
}

int mip_connect(char *sock_path, uint8_t sdu_type)
{
        int sd, rc;

        /* Call the method to create, bind and listen to the server socket */
        sd = init_unix_sock(sock_path, USOCK_CONNECT);
        if (sd < 0)
                return -1;

        /* Send SDU type */
        rc = send_sdu_type(sd, sdu_type);
        if (rc < 0) {
                close(sd);
                return rc;
        }

        return sd;
}

int recv_ping(int sd, struct mip_unix_msg *ping, uint8_t type)
{
        int rc;

        /* Peek at it */
        if ((rc = recv(sd, ping, sizeof(struct mip_unix_msg), MSG_PEEK)) < 0)
                goto recv_error;

        if (type & PING)
                /* If the message is a PING, read it for real */
                if (strncmp(ping->msg, "PING:", 5) == 0)
                        goto recv;

        if (type & PONG)
                /* If the message is a PONG, read it for real */
                if (strncmp(ping->msg, "PONG:", 5) == 0)
                        goto recv;

        return -1;

 recv:
        /* https://xkcd.com/292/ */
        if ((rc = (recv(sd, ping, sizeof(struct mip_unix_msg), 0))) < 0)
                goto recv_error;
        return rc;

 recv_error:
        perror("recv_ping");
        return SOCK_CON_TERM;
}

int send_ping(int unix_sock, struct mip_unix_msg *ping)
{
        return send_data(unix_sock, ping, sizeof(struct mip_unix_msg));
}

int send_sdu_type(int unix_sock, uint8_t type)
{
        return send_data(unix_sock, &type, sizeof(uint8_t));
}

/* More or less just a wrapper around `send()` */
int send_data(int unix_sock, const void *data, size_t len)
{
        int rc;

        if (unix_sock < 0)
                /* If the unix socket, for whatever reason, is not initialized.
                   Perhaps there is no connected application above? */
                return SOCK_NOT_INIT;

        /* Writing message to unix socket, i.e. upper layer application */
        if ((rc = send(unix_sock, data, len, 0)) < 0) {
                if (errno == EPIPE)
                        /* Socket not avalible for writing, likely due to a
                           connection termination */
                        return SOCK_CON_TERM;
                else
                        perror("send_data");
        }
        return rc;
}
