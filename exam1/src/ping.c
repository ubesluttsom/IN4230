#include "ping.h"

int recv_ping(int sd, struct ping *ping, uint8_t type)
{
        int rc;

        /* Peek at it */
        if ((rc = recv(sd, ping, sizeof(struct ping), MSG_PEEK)) < 0)
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
        if ((rc = (recv(sd, ping, sizeof(struct ping), 0))) < 0)
                goto recv_error;
        return rc;

recv_error:
        perror("recv_ping");
        return rc;
}

int send_ping(int unix_sock, struct ping *ping)
{
        int rc;

        if (unix_sock < 0)
                /* If the unix socket, for whatever reason, is not initialized.
                 * Perhaps there is no connected application above? */
                return SOCK_NOT_INIT;

        /* Writing message to unix socket, i.e. upper layer application */
        if ((rc = send(unix_sock, ping, sizeof(struct ping), 0)) < 0) {
                if (errno == EPIPE)
                        /* Socket not avalible for writing, likely due to a
                         * connection termination  */
                        return SOCK_CON_TERM;
                else
                        perror("send_ping");
        }
        return rc;
}

