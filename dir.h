#ifndef DIR_H
#define DIR_H

#include <limits.h>

int dir_foreach(const char *, int (*cb)(const char*, const char *));
void make_path(char *, const char*, const char *);
int dir_is_protected(const char *);

#endif /* DIR_H */
