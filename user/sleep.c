#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: sleep <ticks>\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  char *s;

  if(argc != 2)
    usage();
  s = argv[1];
  if(*s == 0)
    usage();
  for(; *s; s++)
    if(*s < '0' || *s > '9')
      usage();

  pause(atoi(argv[1]));
  exit(0);
}