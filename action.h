#ifndef ACTION_H
#define ACTION_H

struct _connection;

typedef enum 
{ ActionShell, ActionBeep, ActionImage, ActionImageReset } ActionType;

typedef struct _action Action;
struct _action {
  ActionType type;
  char *tag;
  char *cmd;
};

void action_add(ActionType type, const char *tag, const char *cmd);
const Action *action_find(const char *s);
void action_perform(const Action *a, const struct _connection *c);

#endif /* ACTION_H */
