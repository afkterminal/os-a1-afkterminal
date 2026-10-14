#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

#define ANY   0
#define ONLYF 1
#define ONLYD 2

static char path[1024];
static char *pat;
static int type;

// '*' matches any sequence (including empty); everything else is literal.
static int
match(char *p, char *s)
{
  if(*p == 0)
    return *s == 0;
  if(*p == '*'){
    for(;; s++){
      if(match(p + 1, s))
        return 1;
      if(*s == 0)
        return 0;
    }
  }
  if(*s && *p == *s)
    return match(p + 1, s + 1);
  return 0;
}

// path[0..len) holds the directory to search.
static void
walk(int len)
{
  int fd, n, r, i;
  struct stat st;
  char *data;

  fd = open(path, 0);
  if(fd < 0)
    return;
  if(fstat(fd, &st) < 0 || st.type != T_DIR){
    close(fd);
    return;
  }
  data = malloc(st.size + 1);
  if(data == 0){
    close(fd);
    return;
  }
  n = 0;
  while(n < st.size && (r = read(fd, data + n, st.size - n)) > 0)
    n += r;
  close(fd);

  for(i = 0; i + (int)sizeof(struct dirent) <= n; i += sizeof(struct dirent)){
    struct dirent *de = (struct dirent *)(data + i);
    char name[DIRSIZ + 1];
    struct stat cst;
    int nl, newlen, isdir, want;

    if(de->inum == 0)
      continue;
    memmove(name, de->name, DIRSIZ);
    name[DIRSIZ] = 0;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    nl = strlen(name);
    if(len + 1 + nl + 1 > (int)sizeof(path)){
      fprintf(2, "find: path too long\n");
      continue;
    }
    newlen = len;
    if(len > 0 && path[len - 1] != '/')
      path[newlen++] = '/';
    memmove(path + newlen, name, nl + 1);
    newlen += nl;

    if(stat(path, &cst) >= 0){
      isdir = (cst.type == T_DIR);
      want = (type == ANY) || (type == ONLYF && !isdir) || (type == ONLYD && isdir);
      if(want && match(pat, name))
        printf("%s\n", path);
      if(isdir)
        walk(newlen);
    }
    path[len] = 0;   // restore the parent path
  }
  free(data);
}

static void
usage(void)
{
  fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  int fd;

  if(argc == 3)
    type = ANY;
  else if(argc == 5 && strcmp(argv[3], "-type") == 0 &&
          (strcmp(argv[4], "f") == 0 || strcmp(argv[4], "d") == 0))
    type = (argv[4][0] == 'f') ? ONLYF : ONLYD;
  else
    usage();

  if(strlen(argv[1]) >= sizeof(path) - 1){
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  fd = open(argv[1], 0);
  if(fd < 0){
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  close(fd);

  strcpy(path, argv[1]);
  pat = argv[2];
  walk(strlen(path));
  exit(0);
}
