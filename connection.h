#ifndef CONNECTION_H
#define CONNECTION_H

#include "action.h"
#include "buffer.h"
#include <sys/types.h>

#define MAX_CONNECTION_UPDATES 64
#define MAX_CONNECTION_RESETS 64
#define MAX_CONNECTION_FOLDERS 64
#define MAX_CONNECTIONS 64

#define CSTATE_WAIT_HELLO 0
#define CSTATE_ADDING_FOLDERS 1
#define CSTATE_NORMAL 3
#define CSTATE_PENALTY_BOX 4

typedef struct _folder Folder;
struct _folder {
  char *name;
  int biffed;
};

typedef struct _connection Connection;
struct _connection {
  int state;
  int pending_commands;

  time_t retry_time;
  int penalty;

  int fd_in;
  int fd_out;
  Buffer bin;
  Buffer bout;

  char *name;
  char *command;

  unsigned nupdates;
  const Action *updates[MAX_CONNECTION_UPDATES];

  unsigned nresets;
  const Action *resets[MAX_CONNECTION_RESETS];

  unsigned nfolders;
  Folder folders[MAX_CONNECTION_FOLDERS];
  unsigned folders_added;
};

extern Connection connections[MAX_CONNECTIONS];
extern unsigned nconnections;

void connection_init(Connection *c);
void connection_close(Connection *c);

void connection_do_update(Connection *c, const char *folder);
void connection_do_reset(Connection *c, const char *folder);

#endif /* CONNECTION_H */
