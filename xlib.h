#ifndef XLIB_H
#define XLIB_H

#include <stdlib.h>

void *xmalloc(size_t len);
void *xrealloc(void *p, unsigned long len);
char *xstrdup(const char *s);
void xerror(char *fmt, ...);
void xdie(const char *);
const char *xsyserr(void);

int xpopen2(const char *s, int *in, int *out);
void xsystem(const char *s);

int xchomp(char *s);
int xsplit(char *delim, char *buf, char **resp, int nresp);

#endif /* XLIB_H */
