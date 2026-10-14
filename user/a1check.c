#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/uproc.h"
#include "user/user.h"

static int fails;
static struct uproc procs[64];

static void
check(int ok, char *msg)
{
  if(ok)
    printf("PASS %s\n", msg);
  else {
    printf("FAIL %s\n", msg);
    fails++;
  }
}

// run a program with stdin = file "a1in"; return its exit status
static int
run(char *a, char *b, char *c, char *d)
{
  char *argv[5];
  int n = 0, pid, st;

  argv[n++] = a;
  if(b) argv[n++] = b;
  if(c) argv[n++] = c;
  if(d) argv[n++] = d;
  argv[n] = 0;
  pid = fork();
  if(pid == 0){
    close(0);
    open("a1in", O_RDONLY);
    exec(a, argv);
    exit(99);
  }
  wait(&st);
  return st;
}

int
main(void)
{
  int fd, pid, i, j, n, bad;
  struct uproc one;

  fd = open("a1in", O_CREATE | O_RDWR);
  write(fd, "a b\nc\n", 6);
  close(fd);

  printf("== exit statuses (usage/error text below is expected) ==\n");
  check(run("sleep", "1", 0, 0) == 0, "sleep 1 exits 0");
  check(run("sleep", 0, 0, 0) == 1, "sleep (no arg) exits 1");
  check(run("sleep", "1", "2", 0) == 1, "sleep 1 2 exits 1");
  check(run("sleep", "-1", 0, 0) == 1, "sleep -1 exits 1");
  check(run("sleep", "1x", 0, 0) == 1, "sleep 1x exits 1");
  check(run("find", "nodir", "x", 0) == 1, "find missing dir exits 1");
  check(run("find", ".", "README", 0) == 0, "find . README exits 0");
  check(run("xargs", "-n", "0", "echo") == 1, "xargs -n 0 exits 1");
  check(run("xargs", "echo", 0, 0) == 0, "xargs echo exits 0");
  check(run("xargs", "-n", "1", "echo") == 0, "xargs -n 1 echo exits 0");
  check(run("xargs", "nosuchcmd", 0, 0) == 1, "xargs failing cmd exits 1");

  printf("== trace: look at the lines printed after each header ==\n");
  printf("-- expect: [pid] open(?) -> -1  and  [pid] open(\"README\") -> 3\n");
  pid = fork();
  if(pid == 0){
    trace(1 << 15);
    open((char *)0x7fffffffL, O_RDONLY);
    open("README", O_RDONLY);
    exit(0);
  }
  wait(0);

  printf("-- expect: [parent] fork() -> child, then [child] getpid() -> child\n");
  trace((1 << 1) | (1 << 11));
  pid = fork();
  if(pid == 0){
    getpid();
    exit(0);
  }
  wait(0);
  trace(0);

  printf("-- expect: write(1) lines after exec (mask survives exec), then hi\n");
  pid = fork();
  if(pid == 0){
    char *a[] = {"echo", "hi", 0};
    trace(1 << 16);
    exec("echo", a);
    exit(1);
  }
  wait(0);

  printf("== getprocs ==\n");
  n = getprocs(procs, 64);
  check(n >= 2, "getprocs returns at least 2 entries");
  check(procs[0].pid == 1 && procs[0].ppid == 0, "first entry is init with ppid 0");
  check(getprocs(procs, 1) == 1, "max=1 returns 1");
  check(getprocs(procs, 2) == 2, "max=2 returns 2");
  check(getprocs(procs, 0) == 0, "max=0 returns 0");
  check(getprocs(procs, -1) == -1, "max=-1 returns -1");
  check(getprocs((struct uproc *)0x7fffffffL, 5) == -1, "bad pointer returns -1");
  check(getprocs((struct uproc *)0xffffffffffL, 5) == -1, "huge pointer returns -1");

  bad = 0;
  for(i = 0; i < 8; i++){
    if(fork() == 0){
      for(j = 0; j < 20; j++){
        int p = fork();
        if(p == 0)
          exit(0);
        wait(0);
      }
      exit(0);
    }
  }
  for(i = 0; i < 200; i++){
    n = getprocs(procs, 64);
    if(n < 0){
      bad++;
      continue;
    }
    for(j = 0; j < n; j++)
      if(procs[j].pid <= 0 || procs[j].state < 1 || procs[j].state > 5)
        bad++;
  }
  for(i = 0; i < 8; i++)
    wait(0);
  check(bad == 0, "getprocs under fork/exit stress");

  one.pid = -1;
  n = getprocs(&one, 1);
  check(n == 1 && one.pid == 1, "single-entry copy is init");

  if(fails == 0)
    printf("a1check: ALL PASS\n");
  else
    printf("a1check: %d FAILED\n", fails);
  exit(fails ? 1 : 0);
}