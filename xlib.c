#include "xlib.h"
#include "gui.h"
#include "defaults.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

void *xmalloc(size_t len) {
  char *p;

  p = malloc(len);
  if(!p) {
    xerror("malloc(%lu): %s", len, strerror(errno));
    xdie(NULL);
  }
  return p;
}

void *xrealloc(void *p, unsigned long len) {
  p = realloc(p, len);
  if(!p) {
    xerror("realloc(%lu): %s", len, strerror(errno));
    xdie(NULL);
  }
  return p;
}

char *xstrdup(const char *s) {
  char *ret;
  unsigned long len;
 
 len = strlen(s) + 1;
 ret = xmalloc(len);
 memcpy(ret, s, len);
 return ret;
}

void xerror(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  if(!gui.display_message || gui.display_message(fmt, ap) < 0) {
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
  }
  va_end(ap);
}

void xdie(const char *s) {
  if(s)
    xerror("%s: halting", s);
  exit(111);
}

int xpopen2(const char *s, int *in, int *out) {
  int pipefd[2][2];

  if(pipe(pipefd[0]) < 0)
    return -1;
  if(pipe(pipefd[1]) < 0) {
    close(pipefd[0][0]);
    close(pipefd[0][1]);
    return -1;
  }

  switch(fork()) {
    case -1: 
      close(pipefd[0][0]);
      close(pipefd[0][1]);
      close(pipefd[1][0]);
      close(pipefd[1][1]);
      return -1;
    case 0:
      close(pipefd[0][1]);
      close(pipefd[1][0]);
      close(0);
      close(1);
      dup2(pipefd[0][0], 0);
      dup2(pipefd[1][1], 1);
      execl(SHELL, SHELL, "-c", s, NULL);
      printf("NO Unable to exec %s\n", SHELL);
      exit(1);
    default:
      close(pipefd[0][0]);
      close(pipefd[1][1]);
      *out = pipefd[0][1];
      *in = pipefd[1][0];
      return 0;
  }
}

void xsystem(const char *s) {
  switch(fork()) {
    case -1:
    case 0:
      close(0);
      close(1);
      close(2); 
      execl(SHELL, SHELL, "-c", s, NULL);
      exit(1);
    default:
      return;
  }
}

int xchomp(char *s) {
  char *p = s + strlen(s) - 1;

  if(*p == '\n') {
    *p-- = '\0';
    if(*p == '\r') {
      *p = '\0';
      return 2;
    }
    return 1;
  }
  return 0;
}

int xsplit(char *delim, char *s, char **resp, int nresp) {
  char *p;
  int n;

  if(!nresp)
    return 0;

  resp[0] = s;
  n = 1;
  for(p = s; *p; p++) {
    if(strchr(delim, *p)) {
      *p++ = '\0';
      if(!*p)
        return n;
      resp[n++] = p;
      if(n == nresp)
        return n;
    }
  }
  return n;
}

const char *xsyserr() {
  return strerror(errno);
}
