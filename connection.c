#include "connection.h"
#include "xlib.h"
#include "gui.h"

#include <unistd.h>
#include <string.h>
#include <time.h>

Connection connections[MAX_CONNECTIONS];
unsigned nconnections = 0;

void connection_init(Connection *c) {
  c->state = CSTATE_PENALTY_BOX;
  c->pending_commands = 0;

  c->retry_time = time(0);
  c->penalty = 1;

  c->fd_in = c->fd_out = -1;
  buffer_init(&c->bin);
  buffer_init(&c->bout);

  c->name = NULL;
  c->command = NULL;

  c->nupdates = 0;
  c->nresets = 0;
  c->nfolders = 0;
  c->folders_added = 0;
}

void connection_close(Connection *c) {
  int i;

  if(c->fd_in >= 0)
    close(c->fd_in);
  if(c->fd_out >= 0)
    close(c->fd_out);

  gui.delete_fd(c, GUI_FD_READ|GUI_FD_WRITE);

  buffer_finish(&c->bin);
  buffer_finish(&c->bout);

  c->folders_added = 0;

  c->retry_time = time(0) + 2 * c->penalty;
  c->penalty *= 2;
  c->state = CSTATE_PENALTY_BOX;
  c->pending_commands = 0;

  gui.schedule_retry(c->retry_time);
  
  for(i = 0; i < c->nfolders; i++)
    c->folders[i].biffed = 0;
  gui.flagdown(c);
}

void connection_do_update(Connection *c, const char *folder) {
  int i;

  if(folder) {
    xerror("%s: %s biffed", c->name, folder);
    for(i = 0; i < c->nfolders; i++) {
      if(!strcmp(folder, c->folders[i].name)) {
        c->folders[i].biffed = 1;
        break;
      }
    }
  }

  for(i = 0; i < c->nupdates; i++)
    action_perform(c->updates[i], c);
}

void connection_do_reset(Connection *c, const char *folder) {
  int i;
  int biff_coefficient = 0;
  int found = 0;
  int was_biffed = 0;

  if(folder) {
    for(i = 0; i < c->nfolders; i++) {
      if(!found && !strcmp(folder, c->folders[i].name)) {
        was_biffed = c->folders[i].biffed;
        c->folders[i].biffed = 0;
        found = 1;
      }
      else
        biff_coefficient += c->folders[i].biffed;
    }
  }

  if(!biff_coefficient && found && was_biffed) {
    for(i = 0; i < c->nresets; i++)
      action_perform(c->resets[i], c);
  }
}
