#include <stdio.h>
#include <string.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "child.h"
#include "conf.h"
#include "connection.h"
#include "defaults.h"
#include "gui.h"
#include "home.h"
#include "proto.h"
#include "xlib.h"

static void printhelp(FILE *out) {
    fprintf(out, "Usage: netbiff [options]\n");
    fprintf(out, "\t-c, --conf\tSpecify configuration file "
        "(default: ~/.netbiffrc)\n");
    fprintf(out, "\t-g, --gui\tSpecify the interface to use\n");
    fprintf(out, "\t-h, --help\tThis screen\n");
}

int main(int argc, char **argv) {
  int ch;
  char *conf_dir = NULL;
  char *gui_name = NULL;

  child_ignore();

  {
#ifdef HAVE_GETOPT_LONG
    const static struct option long_opt[] = {
      { "conf", 1, NULL, 'c' },
      { "gui",  1, NULL, 'g' },
      { "help", 0, NULL, 'h' },
      { 0, 0, 0, 0 }
    };
#endif
    static const char short_opt[] = "hc:g:";

#ifdef HAVE_GETOPT_LONG
    while((ch = getopt_long(argc, argv, short_opt, long_opt, NULL)) != EOF) {
#else
    while((ch = getopt(argc, argv, "hc:g:")) != -1) {
#endif
      switch(ch) {
        case 'h': printhelp(stdout); exit(0);
        case 'c': conf_dir = xstrdup(optarg); break;
        case 'g': gui_name = optarg; break;
        case '?': printhelp(stderr); exit(1);
      }
    }
  }

  if(gui_init(&argc, &argv, gui_name) < 0)
    xdie("unable to init interface");

  if(!conf_dir) {
    const char* home;

    home = find_home();
    if(!home)
      xdie("Unable to locate home directory");

    conf_dir = xmalloc(strlen(DEFAULT_CONF_DIR) + strlen(home) + 2);
    strcpy(conf_dir, home);
    strcat(conf_dir, "/");
    strcat(conf_dir, DEFAULT_CONF_DIR);
  }

  conf_set_base(conf_dir);
  free(conf_dir);

  if(conf_read_options() < 0 ||
      conf_read_actions() < 0 ||
      conf_read_connections() < 0)
    xdie("unable to read configuration");

  gui.register_connections();

  proto_do_retries();

  gui.event_loop();

  return 0;
}
