#include "child.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

static void child_handler() {
  signal(SIGCHLD, child_handler);
  while(waitpid(-1, NULL, WNOHANG) > 0) ;
}

void child_ignore() {
  signal(SIGCHLD, child_handler);
}
