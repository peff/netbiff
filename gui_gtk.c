#include "gtk.h"
#include "gui.h"

#ifndef GUI_GTK

GUI *gui_gtk_description = NULL;

#else

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "conf.h"
#include "connection.h"
#include "proto.h"
#include "xlib.h"

#include "flagup.xpm"
#include "flagdown.xpm"
#include "yes.xpm"
#include "no.xpm"

static GdkPixmap *flagup;
static GdkBitmap *upmask;
static GdkPixmap *flagdown;
static GdkBitmap *downmask;

static GdkPixmap *yes;
static GdkBitmap *yesmask;
static GdkPixmap *no;
static GdkBitmap *nomask;

static GtkWidget *main_window;
static GtkWidget *flag;

static GtkWidget *status_window;
static int status_shown = 0;
static GtkWidget *status_table;
static GtkWidget *status_statusbar;
static guint status_size;
static guint status_context;

static GtkWidget *connection_images[MAX_CONNECTIONS];

static void gtk_flagup(const Connection *c) {
  if(c) 
    gtk_pixmap_set(GTK_PIXMAP(connection_images[c-connections]), yes, yesmask);
  gtk_pixmap_set(GTK_PIXMAP(flag), flagup, upmask);
}

static void gtk_flagdown(const Connection *c) {
  int i, j;
  int biffed;

  if(c) 
    gtk_pixmap_set(GTK_PIXMAP(connection_images[c-connections]), no, nomask);

  biffed = 0;
  for(i = 0; i < nconnections; i++) {
    for(j = 0; j < connections[i].nfolders; j++) {
      if(connections[i].folders[j].biffed) {
        biffed++;
      }
    }
  }

  if(!biffed)
    gtk_pixmap_set(GTK_PIXMAP(flag), flagdown, downmask);
}

static gint hide_status() {
  status_shown = 0;
  gtk_widget_hide(status_window);
  return TRUE;
}

static gint show_status() {
  int winx, winy;

  status_shown = 1;
  
  gdk_window_get_position(main_window->window, &winx, &winy);
  gtk_widget_set_uposition(status_window,
      winx + main_window->allocation.width,
      winy + main_window->allocation.height);
  
  gtk_widget_show_all(status_window);

  return TRUE;
}

static gint image_clicked(GtkWidget *widget, GdkEventButton *event,
    gpointer data) {
  switch(event->button) {
    case 1:
      gtk_pixmap_set(GTK_PIXMAP(flag), flagdown, downmask);
      break;
    case 3:
      if(status_shown)
        hide_status();
      else
        show_status();
      break;
  }
  return TRUE;
}

static gint statusbar_clicked(GtkWidget *widget, GdkEventButton *event, 
    gpointer data) {
  switch(event->button) {
    case 1:
      gtk_statusbar_pop(GTK_STATUSBAR(status_statusbar), status_context);
      break;
    case 2:
      while(status_size) {
        gtk_statusbar_pop(GTK_STATUSBAR(status_statusbar), status_context);
        status_size--;
      }
      break;
  }
  return TRUE;
}

void main_window_create() {
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(main_window), "delete_event",
      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_container_border_width(GTK_CONTAINER(main_window), 0);
  gtk_window_set_title(GTK_WINDOW(main_window), "NetBiff");

  gtk_widget_set_events(main_window, 
      gtk_widget_get_events(main_window) | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(main_window), "button_press_event",
      GTK_SIGNAL_FUNC(image_clicked), NULL); 

  gtk_widget_realize(main_window);
}

void pixmap_create() {
  GtkStyle *style;
  style = gtk_widget_get_style(main_window);

  flagdown = gdk_pixmap_create_from_xpm_d(main_window->window,
                                          &downmask,
                                          &style->black,
                                          (gchar **)flagdown_data);
  flagup = gdk_pixmap_create_from_xpm_d(main_window->window,
                                         &upmask,
                                         &style->white,
                                         (gchar **)flagup_data);
  
  flag = gtk_pixmap_new(flagdown, downmask);
  gtk_container_add(GTK_CONTAINER(main_window), flag);

  yes = gdk_pixmap_create_from_xpm_d(main_window->window,
      &yesmask, style->bg, (gchar **)yes_xpm);
  no = gdk_pixmap_create_from_xpm_d(main_window->window,
      &nomask, style->bg, (gchar **)no_xpm);
}

void status_window_create() {
  GtkWidget *box;

  status_window = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_container_border_width(GTK_CONTAINER(status_window), 2);
  gtk_window_set_transient_for(GTK_WINDOW(status_window), 
      GTK_WINDOW(main_window));
  gtk_window_set_policy(GTK_WINDOW(status_window), 1, 1, 1);
  gtk_signal_connect(GTK_OBJECT(status_window), "delete_event",
      GTK_SIGNAL_FUNC(hide_status), NULL);

  box = gtk_vbox_new(0, FALSE);
  gtk_container_add(GTK_CONTAINER(status_window), box);

  status_table = gtk_table_new(1, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(status_table), 0);
  gtk_table_set_col_spacings(GTK_TABLE(status_table), 5);
  gtk_box_pack_start(GTK_BOX(box), status_table, FALSE, FALSE, 0);

  gtk_table_attach_defaults(GTK_TABLE(status_table),
      gtk_label_new("Connection"), 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(status_table), 
      gtk_label_new("Status"), 1, 2, 0, 1);

  status_statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(box), status_statusbar, FALSE, FALSE, 0);
  status_context = gtk_statusbar_get_context_id(
      GTK_STATUSBAR(status_statusbar), "Netbiff");
  
  gtk_widget_set_events(status_window,
      gtk_widget_get_events(status_window) | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(status_window), "button_press_event",
      GTK_SIGNAL_FUNC(statusbar_clicked), NULL);
}

static void gtk_gui_init(int *argc, char ***argv) {
  char **a;

  gtk_init(argc, argv);
  /* The original argv may not be available, so gtk's guess for the
   * program class will be wrong. */
  gdk_set_program_class("Netbiff");

  main_window_create();
  pixmap_create();
  status_window_create();

  a = *argv;
  while(*++a) {
    if(!strcmp(*a, "-geometry")) {
      a++;
      if(!*a) {
        xerror("-geometry requires value on command line");
        continue;
      }
      else {
        if(!gtk_window_parse_geometry(GTK_WINDOW(main_window), *a)) {
          xerror("invalid format for -geometry");
          continue;
        }
      }
    }
    else if(!strcmp(*a, "--"))
      continue;
    else {
      xerror("Unrecognized option: %s", *a);
    }
  }
}

static int gtk_display_message(const char *fmt, va_list ap) {
  char buf[4096];
  unsigned long len;
  time_t t;
  const struct tm *tm;
  
  t = time(NULL);
  tm = localtime(&t);
  len = strftime(buf, sizeof(buf), "%H:%M:%S ", tm);
  vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);

  if(GTK_WIDGET_VISIBLE(main_window)) {
    gtk_statusbar_push(GTK_STATUSBAR(status_statusbar), status_context, buf);
    status_size++;
  }
  else {
    fputs(buf, stderr);
    putc('\n', stderr);
  }

  return 0;
}

static int gtk_poll_shim(void *data) {
  proto_do_poll();
  return TRUE;
}

static int gtk_retry_shim(void *data) {
  proto_do_retries();
  return FALSE;
}

static void gtk_event_loop() {
  gtk_timeout_add(poll_frequency * 1000, gtk_poll_shim, NULL);
  gtk_widget_show_all(main_window);
  gtk_main();
}

static void gtk_beep() {
  gdk_beep();
}

struct taglist {
  const Connection *c;
  int type;
  int tag;
  struct taglist *next;
  struct taglist *prev;
};
static struct taglist *taghead, *tagtail = NULL;

static void taglist_add(const Connection *c, int type, int tag) {
  struct taglist *t;

  t = xmalloc(sizeof(struct taglist));
  t->c = c;
  t->type = type;
  t->tag = tag;
  t->prev = tagtail;
  t->next = NULL;
  if(tagtail)
    tagtail->next = t;
  else
    taghead = t;
  tagtail = t;
}

static int taglist_find_and_delete(const Connection *c, int type) {
  struct taglist *t;

  for(t = taghead; t; t = t->next) {
    if(t->c == c && t->type == type) {
      int tag = t->tag;
      if(t->next)
        t->next->prev = t->prev;
      else
        tagtail = t->prev;
      if(t->prev) 
        t->prev->next = t->next;
      else
        taghead = t->next;
      free(t);
      return tag;
    }
  }
  return -1;
}

static void gtk_io_shim(Connection *c, gint fd, GdkInputCondition cond) {
  switch(cond) {
    case GDK_INPUT_READ: proto_do_conn_input(c); break;
    case GDK_INPUT_WRITE: proto_do_conn_output(c); break;
    default: break;
  }
}

static void gtk_add_fd(const Connection *c, int type) {
  if(type & GUI_FD_READ) {
    int tag;

    tag = gdk_input_add(c->fd_in, GDK_INPUT_READ,
        (GdkInputFunction)gtk_io_shim, (gpointer)c);
    taglist_add(c, GUI_FD_READ, tag);
  }
  if(type & GUI_FD_WRITE) {
    int tag;

    tag = gdk_input_add(c->fd_out, GDK_INPUT_WRITE,
       (GdkInputFunction)gtk_io_shim, (gpointer)c);
    taglist_add(c, GUI_FD_WRITE, tag);
  }
}

static void gtk_delete_fd(const Connection *c, int type) {
  int tag;

  if(type & GUI_FD_READ) {
    tag = taglist_find_and_delete(c, GUI_FD_READ);
    if(tag >= 0)
      gdk_input_remove(tag);
  }
  if(type & GUI_FD_WRITE) {
    tag = taglist_find_and_delete(c, GUI_FD_WRITE);
    if(tag >= 0)
      gdk_input_remove(tag);
  }
}

static const char *request_buffer = NULL;
static void request_callback(GtkWidget *w, gpointer data) {
  request_buffer = gtk_entry_get_text(GTK_ENTRY(w));
}

static gint request_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  request_buffer = "";
  return TRUE;
}


static int pending_request = 0;
static char *gtk_request_data(const char *prompt, int hidden) {
  GtkWidget *dialog;
  GtkWidget *input_box;
  GtkWidget *label;
  char *retval;

  if(pending_request)
    return NULL;

  pending_request = 1;

  dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", 
      GTK_SIGNAL_FUNC(request_delete), NULL);

  label = gtk_label_new("A module has requested the data:");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), 
      label, 0, FALSE, FALSE);
  label = gtk_label_new(prompt);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), 
      label, 0, FALSE, FALSE);

  input_box = gtk_entry_new();
  if(hidden)
    gtk_entry_set_visibility(GTK_ENTRY(input_box), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(input_box), TRUE);
  gtk_signal_connect(GTK_OBJECT(input_box), "activate", 
      GTK_SIGNAL_FUNC(request_callback), NULL);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
      input_box, 0, TRUE, TRUE);

  gtk_widget_show_all(dialog);
  gtk_grab_add(dialog);

  while(!request_buffer)
    g_main_iteration(TRUE);

  retval = strdup(request_buffer);

  gtk_grab_remove(dialog);
  gtk_widget_destroy(dialog);

  request_buffer = NULL;
  pending_request = 0;

  return retval;
}

static void gtk_register_connections() {
  int i;

  for(i = 0; i < nconnections; i++) {
    connection_images[i] = gtk_pixmap_new(no, nomask);
    gtk_table_attach_defaults(GTK_TABLE(status_table), 
        gtk_label_new(connections[i].name),
        0, 1, 
        i+1, i+2);
    gtk_table_attach_defaults(GTK_TABLE(status_table), 
        connection_images[i],
        1, 2, 
        i+1, i+2);
  }
}

static void gtk_schedule_retry(time_t when) {
  time_t now;

  now = time(0);

  if(when < now)
    when = now;

  gtk_timeout_add((when - now) * 1000, gtk_retry_shim, NULL);
}

static GUI gui_gtk = {
  gtk_flagup,
  gtk_flagdown,
  gtk_gui_init,
  gtk_display_message,
  gtk_event_loop,
  gtk_beep,
  gtk_add_fd,
  gtk_delete_fd,
  gtk_request_data,
  gtk_register_connections,
  gtk_schedule_retry
};
GUI *gui_gtk_description = &gui_gtk;

#endif /* GUI_GTK */
