#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

static void
check(int ok, char *msg)
{
  if(!ok){
    printf("sysinfotest: FAIL %s\n", msg);
    exit(1);
  }
}

int
main(void)
{
  struct sysinfo a, b;
  int fd, p[2], pid, i;
  char *mem;

  // basic call works
  check(sysinfo(&a) == 0, "sysinfo returned error");

  // freemem goes down when memory is allocated
  mem = sbrk(8 * 4096);
  for(i = 0; i < 8; i++)
    mem[i * 4096] = 1;
  check(sysinfo(&b) == 0, "sysinfo after sbrk");
  check(b.freemem < a.freemem, "freemem did not go down");
  sbrk(-8 * 4096);

  // nproc goes up after fork
  check(sysinfo(&a) == 0, "sysinfo before fork");
  pid = fork();
  if(pid == 0){
    pause(30);
    exit(0);
  }
  check(sysinfo(&b) == 0, "sysinfo after fork");
  check(b.nproc == a.nproc + 1, "nproc did not go up after fork");
  wait(0);

  // nopenfile changes with open, pipe and close
  check(sysinfo(&a) == 0, "sysinfo before open");
  fd = open("README", 0);
  check(fd >= 0, "open README failed");
  check(sysinfo(&b) == 0, "sysinfo after open");
  check(b.nopenfile == a.nopenfile + 1, "nopenfile after open");
  check(pipe(p) == 0, "pipe failed");
  check(sysinfo(&b) == 0, "sysinfo after pipe");
  check(b.nopenfile == a.nopenfile + 3, "nopenfile after pipe");
  close(fd);
  close(p[0]);
  close(p[1]);
  check(sysinfo(&b) == 0, "sysinfo after close");
  check(b.nopenfile == a.nopenfile, "nopenfile after close");

  // an invalid pointer must fail, not crash
  check(sysinfo((struct sysinfo *)0xffffffffffL) == -1, "bad pointer not rejected");

  printf("sysinfotest: OK\n");
  exit(0);
}