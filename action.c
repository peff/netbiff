#include "action.h"
#include "xlib.h"
#include "gui.h"
#include "connection.h"

#include <stdlib.h>
#include <string.h>

#define MAX_ACTIONS 128
static Action actions[MAX_ACTIONS] = {
  { ActionBeep, "beep", NULL },
  { ActionImage, "image", NULL },
  { ActionImageReset, "image-reset", NULL }
};
int nactions = 3;

void action_add(ActionType type, const char *tag, const char *cmd) {
  actions[nactions].type = type;
  actions[nactions].tag = xstrdup(tag);
  actions[nactions].cmd = xstrdup(cmd);
  nactions++;
}

const Action *action_find(const char *s) {
  int i;
  for(i = 0; i < nactions; i++) 
    if(!strcasecmp(s, actions[i].tag))
      return actions + i;
  return NULL;
}

void action_perform(const Action *a, const Connection *c) {
  switch(a->type) {
    case ActionBeep:
      gui.beep();
      break;
    case ActionImage:
      gui.flagup(c);
      break;
    case ActionImageReset:
      gui.flagdown(c);
      break;
    case ActionShell:
      xsystem(a->cmd);
      break;
  }
}
