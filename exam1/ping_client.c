#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>  /* sockets operations */
#include <sys/un.h>      /* definitions for UNIX domain sockets */

#include "ping.h"

void client(char* buf, char* socket_name)
{
  struct sockaddr_un addr;
  int    sd, rc;

  sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  // strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
  strncpy(addr.sun_path, socket_name, sizeof(addr.sun_path) - 1);

  rc = connect(sd, (struct sockaddr *)&addr, sizeof(addr));
  if ( rc < 0) {
    perror("connect");
    close(sd);
    exit(EXIT_FAILURE);
  }

  printf("\n*** Sending ping ... ***\n");

  rc = write(sd, buf, strlen(buf));
  if (rc < 0) {
    perror("write");
    close(sd);
    exit(EXIT_FAILURE);
  }
}

void print_usage(char* cmd)
{
    printf("usage: %s [-h] <destination_host> <message> <socket_lower>", cmd);
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
  } else if (argc == 4) {
    client(argv[2], argv[3]);
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
