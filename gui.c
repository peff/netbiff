#include "gui.h"
#include "xlib.h"

#include <stdlib.h>
#include <string.h>

extern GUI *gui_text_description;
extern GUI *gui_gtk_description;

int gui_init(int *argc, char ***argv, const char *name) {
  const GUI *g = NULL;

  if(name) {
    if(!strcasecmp(name, "text"))
      g = gui_text_description;
    else if(!strcasecmp(name, "gtk"))
      g = gui_gtk_description;
    else {
      xerror("Error! Unknown interface: %s", gui);
      xerror("Halting...");
      return -1;
    }
  }

  if(!g) {
    char *disp;

    /* If there's no display, we always go with text. */
    disp = getenv("DISPLAY");
    if(!disp) {
      if(!gui_text_description) {
        xerror("Error! No DISPLAY is set and no text support was compiled in!");
        xerror("Halting...");
        return -1;
      }
      g = gui_text_description;
    }
    else {
      if(gui_gtk_description)
        g = gui_gtk_description;
      else if(gui_text_description)
        g = gui_text_description;
      else {
        xerror("Error! No interface support was compiled in!");
        xerror("Halting...");
        return -1;
      }
    }
  }
  gui = *g;
  gui.init(argc, argv);
  return 0;
}

