#include "util.h"

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

static int resolve(const char *host, struct in_addr *saddr) {
  struct hostent *hent;

  if(inet_aton(host, saddr))
    return 0;

  hent = gethostbyname(host);
  if(!hent) 
    return -1;

  memcpy(saddr, hent->h_addr_list[0], sizeof(struct in_addr));
  return 0;
}

int util_iconnect(const char *host, unsigned short port) {
  int fd;
  struct sockaddr_in s;

  memset(&s, 0, sizeof(s));
  s.sin_family = AF_INET;
  if(resolve(host, &s.sin_addr) < 0)
    return -2;
  s.sin_port = htons(port);

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0)
    return -1;

  if(connect(fd, (struct sockaddr *)&s, sizeof(s)) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

pid_t util_pipeto(char * const cmd[], int *read, int *write) {
  int tochild[2];
  int fromchild[2];
  pid_t pid;

  if(pipe(tochild) < 0) 
    return 0;
  if(pipe(fromchild) < 0) {
    close(tochild[0]);
    close(tochild[1]);
    return 0;
  }

  pid = fork();
  if(pid < 0) {
    close(tochild[0]);
    close(tochild[1]);
    close(fromchild[0]);
    close(fromchild[1]);
    return 0;
  }

  else if(pid == 0) {
    close(tochild[1]);
    close(fromchild[0]);
    if(dup2(tochild[0], 0) < 0)
      exit(1);
    if(dup2(fromchild[1], 1) < 0)
      exit(1);
    if(execvp(cmd[0], cmd) < 0)
      exit(1);
  }

  close(tochild[0]);
  close(fromchild[1]);
  *read = fromchild[0];
  *write = tochild[1];
  
  return pid;
}

void util_chomp(char *buf) {
  char *p;

  p = buf + strlen(buf) - 1;
  if(*p == '\n') {
    *p-- = '\0';
    if(*p == '\r')
      *p = '\0';
  }
}

int util_split(const char *delim, char *buf, char **resp, int max) {
  int count;
  char *p;

  if(max < 1)
    return 0;

  if(!*buf) {
    resp[0] = NULL;
    return 0;
  }

  count = 1;
  resp[0] = p = buf;

  while(*p) {
    if(count >= max)
      return count;
    if(strchr(delim, *p)) {
      do {
        *p++ = '\0';
      } while(*p && strchr(delim, *p));
      if(*p)
        resp[count++] = p;
    }
    else
      p++;
  }
  return count;
}
