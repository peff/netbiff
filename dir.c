#include "dir.h"
#include "xlib.h"

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>

int dir_foreach(const char* dirname, int (*cb)(const char*, const char *)) {
  DIR *d;
  struct dirent *ent;
  int errors;

  d = opendir(dirname);
  if(!d)
    return -1;

  errors = 0;
  while((ent = readdir(d)) != NULL) {
    char buf[PATH_MAX];

    if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
      continue;

    make_path(buf, dirname, ent->d_name);
    if(cb(ent->d_name, buf) < 0)
      errors++;
  }

  closedir(d);
  return errors;
}

/* out is assumed to be of size PATH_MAX */
void make_path(char *out, const char *a, const char *b) {
  int alen, blen;

  alen = strlen(a);
  blen = strlen(b);

  if(alen + blen + 2 > PATH_MAX)
    xdie("attempted buffer overflow!?");

  memmove(out, a, alen);
  out[alen] = '/';
  memmove(out + alen + 1, b, blen);
  out[alen + blen + 1] = '\0';
}

int dir_is_protected(const char* d) {
  struct stat sb;

  if(stat(d, &sb) < 0) {
    if(errno == ENOENT)
      return 1;
    xerror("unable to stat %s: %s", d, xsyserr());
    return 0;
  }

  return sb.st_mode & (S_IRWXG|S_IRWXO) ? 0 : 1;
}
