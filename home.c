#include "home.h"
#include "xlib.h"

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

static const char *home;

const char* find_home() {
  struct passwd *pwinfo;
  
  if(!home)
    home = getenv("HOME");

  if(!home) {
    pwinfo = getpwuid(getuid());
    if(pwinfo)
      home = xstrdup(pwinfo->pw_dir);
  }

  return home;
}
