#ifndef GUI_H
#define GUI_H

#include <stdarg.h>
#include <time.h>

#include "connection.h"

typedef struct {
  void (*flagup)(const Connection *);
  void (*flagdown)(const Connection *);
  void (*init)(int *, char ***);
  int (*display_message)(const char *, va_list);
  void (*event_loop)(void);
  void (*beep)(void);
  void (*add_fd)(const Connection *, int);
  void (*delete_fd)(const Connection *, int);
  char* (*request_data)(const char *, int);
  void (*register_connections)(void);
  void (*schedule_retry)(time_t);
} GUI;

int gui_init(int *argc, char ***argv, const char *name);

GUI gui;

#define GUI_FD_READ  0x1
#define GUI_FD_WRITE 0x2

#endif /* GUI_H */
