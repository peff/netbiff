#include "nbproto.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static char input[4096];
static const char *status_messages[] = {
  "OK",
  "NO",
  "BAD",
  "* UPDATE",
  "* RESET",
  "* DATAREQUEST"
};

void netbiff_send(int status, char *fmt, ...) {
  va_list ap;

  fputs(status_messages[status], stdout);
  fputc(' ', stdout);

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);
  fflush(stdout);
}

int netbiff_proto_next(char *s[]) {
  char *argv[2];
  int argc;
  int i;

  for(i = 0; i < NETBIFF_PROTO_MAXARGS; i++)
    s[i] = NULL;

  while(1) {
    if(!fgets(input, 4096, stdin))
      return NETBIFF_CMD_EOF;

    util_chomp(input);
    argc = util_split(" ", input, argv, 2);
    if(!argc)
      continue;
    
    if(!strcasecmp(argv[0], "POLL")) {
      if(argc > 1)
        s[0] = argv[1];
      return NETBIFF_CMD_POLL;
    }
    else if(!strcasecmp(argv[0], "FOLDER")) {
      if(argc < 2) {
        netbiff_send(BAD, "Command FOLDER requires an argument");
        continue;
      }
      s[0] = argv[1];
      return NETBIFF_CMD_FOLDER;
    }
    else if(!strcasecmp(argv[0], "QUIT")) {
      netbiff_send(OK, "BYE");
      fclose(stdout);
      fclose(stdin);
      return NETBIFF_CMD_QUIT;
    }
    else if(!strcasecmp(argv[0], "DATARESPONSE")) {
      if(argc < 2 || util_split(" ", argv[1], s, 2) < 2) {
        netbiff_send(BAD, "Command DATARESPONSE requires at least two"
            " arguments.\n");
        continue;
      }
      return NETBIFF_CMD_DATARESPONSE;
    }
    else {
      netbiff_send(BAD, "%s unimplemented.", argv[0]);
    }
  }
}
