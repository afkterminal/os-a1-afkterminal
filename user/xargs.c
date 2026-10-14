#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static char *args[MAXARG];
static char buf[4096];
static int base, nw, bp, words, nflag;
static int status;

static void
usage(void)
{
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
  exit(1);
}

static void
toomany(void)
{
  fprintf(2, "xargs: too many arguments\n");
  exit(1);
}

// Run the command with the words collected so far, then reset.
static void
run(void)
{
  int pid, st;

  if(words == 0)
    return;
  args[nw] = 0;
  pid = fork();
  if(pid < 0){
    fprintf(2, "xargs: fork failed\n");
    status = 1;
  } else if(pid == 0){
    exec(args[0], args);
    fprintf(2, "xargs: exec %s failed\n", args[0]);
    exit(1);
  } else {
    wait(&st);
    if(st != 0)
      status = 1;
  }
  nw = base;
  bp = 0;
  words = 0;
}

int
main(int argc, char *argv[])
{
  int i, cmd = 1, inword = 0;
  char c;

  if(argc > 1 && strcmp(argv[1], "-n") == 0){
    if(argc < 4)
      usage();
    if(argv[2][0] == 0)
      usage();
    for(i = 0; argv[2][i]; i++)
      if(argv[2][i] < '0' || argv[2][i] > '9')
        usage();
    nflag = atoi(argv[2]);
    if(nflag < 1)
      usage();
    cmd = 3;
  }
  if(cmd >= argc)
    usage();

  for(i = cmd; i < argc; i++){
    if(base >= MAXARG - 1)
      toomany();
    args[base++] = argv[i];
  }
  nw = base;

  while(read(0, &c, 1) == 1){
    if(c == ' ' || c == '\t' || c == '\n'){
      if(inword){
        buf[bp++] = 0;
        inword = 0;
        if(nflag && words == nflag)
          run();
      }
      if(c == '\n' && !nflag)
        run();
    } else {
      if(!inword){
        if(nw >= MAXARG - 1)
          toomany();
        args[nw++] = &buf[bp];
        words++;
        inword = 1;
      }
      if(bp >= (int)sizeof(buf) - 1)
        toomany();
      buf[bp++] = c;
    }
  }
  if(inword)
    buf[bp++] = 0;
  run();
  exit(status);
}
