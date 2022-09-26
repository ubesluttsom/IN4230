#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
  int opt, lflag = 0, dflag = 0, hflag = 0;

  while ((opt = getopt(argc, argv, "ldh")) != -1) {
    switch (opt) {
      case 'l':
        lflag = 1;
        break;
      case 'd':
        dflag = 1;
        break;
      case 'h':
      default:
        hflag = 1;
        break;
    }
  }

  if (lflag)
    printf("Lorem Ipsum");
  else if (dflag)
    printf("Dolor Sit Amet");
  else {
    printf("usage: %s "
        "[-l] lorem ipsum "
        "[-d] dolor sit amet\n", argv[0]);
    if (hflag == 0) {
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
