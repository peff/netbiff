#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

int util_iconnect(const char *host, unsigned short port);
pid_t util_pipeto(char * const cmd[], int *read, int *write);

void util_chomp(char *buf);
int util_split(const char *delim, char *buf, char **resp, int max);

#endif /* UTIL_H */
