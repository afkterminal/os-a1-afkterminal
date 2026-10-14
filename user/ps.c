#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/uproc.h"
#include "user/user.h"

static struct uproc procs[64];   // static: user stacks are only one page

static char *states[] = {
  "unused", "used", "sleeping", "runnable", "running", "zombie"
};

int
main(void)
{
  int n, i;

  n = getprocs(procs, 64);
  if(n < 0){
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }
  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");
  for(i = 0; i < n; i++){
    char *st = "?";
    if(procs[i].state >= 0 && procs[i].state <= 5)
      st = states[procs[i].state];
    printf("%d\t%d\t%s\t%lu\t%s\n", procs[i].pid, procs[i].ppid, st,
           procs[i].sz, procs[i].name);
  }
  exit(0);
}