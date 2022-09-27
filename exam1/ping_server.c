#include "ping.h"
#include <stdio.h>		   /* standard input/output library functions */
#include <stdlib.h>		   /* standard library definitions (macros) */
#include <string.h>		   /* string operations (strncpy, memset..) */
#include <sys/epoll.h>   /* epoll */
#include <sys/socket.h>  /* sockets operations */
#include <sys/un.h>      /* definitions for UNIX domain sockets */
#include <unistd.h>		   /* standard symbolic constants and types */

/**
 * Prepare (create, bind, listen) the server listening socket
 *
 * Returns the file descriptor of the server socket.
 */
static int prepare_server_sock(char* socket_name)
{
	struct sockaddr_un addr;
	int sd = -1, rc = -1;

	/* 1. Create local UNIX socket of type SOCK_SEQPACKET. */
	
	sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sd  == -1) {
		perror("socket()");
		return rc;
	}

	/*
	 * For portability clear the whole structure, since some
	 * implementations have additional (nonstandard) fields in
	 * the structure.
	 */

	memset(&addr, 0, sizeof(struct sockaddr_un));

	/* 2. Bind socket to socket name. */
	
	addr.sun_family = AF_UNIX;

	/* Why `-1`??? Check character arrays in C!
	 * 's' 'e' 'r' 'v' 'e' 'r' '.' 's' 'o' 'c' 'k' 'e' 't' '\0'
	 * sizeof() counts the null-terminated character ('\0') as well.
	 */
	strncpy(addr.sun_path, socket_name, sizeof(addr.sun_path) - 1);

	/* Unlink the socket so that we can reuse the program.
	 * This is bad hack! Better solution with a lock file,
	 * or signal handling.
	 * Check https://gavv.github.io/articles/unix-socket-reuse
	 */
	unlink(socket_name);

	rc = bind(sd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		perror("bind");
		close(sd);
		return rc;
	}

	/*
	 * 3. Prepare for accepting incomming connections.
	 * The backlog size is set to MAX_CONNS.
	 * So while one request is being processed other requests
	 * can be waiting.
	 */

	rc = listen(sd, MAX_CONNS);
	if (rc == -1) {
		perror("listen()");
		close(sd);
		return rc;
	}

	return sd;
}

static int add_to_epoll_table(int efd, struct epoll_event *ev, int fd)
{
	int rc = 0;

	ev->events = EPOLLIN;
	ev->data.fd = fd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, ev) == -1) {
		perror("epoll_ctl");
		rc = -1;
	}

	return rc;
}

static void handle_client(int fd)
{
	char buf[256];
	int rc;

	/* The memset() function fills the first 'sizeof(buf)' bytes
	 * of the memory area pointed to by 'buf' with the constant byte 0.*/
	memset(buf, 0, sizeof(buf));

  // Peek at the message in the file descriptor.
	rc = recv(fd, buf, sizeof(buf), MSG_PEEK);

  // If the message is a PING, read it for real.
  if (strncmp(buf, "PING:", 5) == 0) {
    rc = recv(fd, buf, sizeof(buf), 0);
  }

	if (rc <= 0) {
		close(fd);
		return;
	}

	printf("<%d>: %s\n", fd, buf);
}

void server(char* socket_name)
{
	struct epoll_event ev, events[MAX_EVENTS];
	int    sd, accept_sd, epollfd, rc;

	printf("\n*** Ping server ***\n"
	       "* Waiting for pings to pong ... *\n\n");

	/* Call the method to create, bind and listen to the server socket */
	sd = prepare_server_sock(socket_name);

	/* Create the main epoll file descriptor */
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		close(sd);
		exit(EXIT_FAILURE);
	}

	/* Add the main listening socket to epoll table */
	rc = add_to_epoll_table(epollfd, &ev, sd);
	if (rc == -1) {
		close(sd);
		exit(EXIT_FAILURE);
	}

	while (1) {
		rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (rc == -1) {
			perror("epoll_wait");
			close(sd);
			exit(EXIT_FAILURE);
		} else if (events->data.fd == sd) {
			/* someone is trying to connect() */
			accept_sd = accept(sd, NULL, NULL);
			if (accept_sd == -1) {
				perror("accept");
				continue;
			}

			/* Add the new socket to epoll table */
			rc = add_to_epoll_table(epollfd, &ev, accept_sd);
			if (rc == -1) {
				close(sd);
				exit(EXIT_FAILURE);
			}
		} else {
			/* existing user is trying to send data */
			handle_client(events->data.fd);
		}
	}

	close(sd);

	/* Unlink the socket. */
	unlink(socket_name);

	exit(EXIT_SUCCESS);
}

void print_usage(char* cmd)
{
  printf("usage: %s [-h] <socket_lower>\n", cmd);
}

int main (int argc, char *argv[])
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
