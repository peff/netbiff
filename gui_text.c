#include "gui.h"

GUI *gui_text_description = NULL;

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "connection.h"
#include "proto.h"
#include "xlib.h"
#include "conf.h"

static fd_set rfds;
static fd_set wfds;
static int maxfd;

static void text_flagup(const Connection *c) {
  printf("The flag is going up for connection %s!\n", c->name);
}

static void text_flagdown(const Connection *c) {
  printf("The flag is going down for connection %s!\n", c->name);
}

static void text_init(int *argc, char ***argv) {
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  maxfd = -1;
  signal(SIGPIPE, SIG_IGN);
}

static int text_display_message(const char *fmt, va_list ap) {
  char buf[128];
  struct tm *tm;
  time_t t;

  t = time(NULL);
  tm = localtime(&t);
  strftime(buf, 128, "[%b %d %H:%M:%S] ", tm);
  fputs(buf, stdout);
  vfprintf(stdout, fmt, ap);
  fputc('\n', stdout);
  fflush(stdout);
  return 0;
}

static unsigned text_calc_timeout(time_t next_poll) {
  time_t when;
  time_t now;
  int i;

  when = next_poll;
  for(i = 0; i < nconnections; i++) {
    if(connections[i].state == CSTATE_PENALTY_BOX) {
      if(connections[i].retry_time < when) {
        when = connections[i].retry_time;
      }
    }
  }

  now = time(NULL);
  return (when < now) ? (0) : (when - now);
}

static void text_event_loop() {
  time_t next_poll;

  next_poll = time(NULL);

  while(1) {
    fd_set r, w;
    struct timeval tv;

    r = rfds;
    w = wfds;
    tv.tv_usec = 0;
    tv.tv_sec = text_calc_timeout(next_poll);

    if(select(maxfd, &r, &w, NULL, &tv) < 0) {
      if(errno != EINTR) {
        xerror("select(): %s\n", strerror(errno));
        return;
      }
      continue;
    }

    proto_do_retries();
    proto_do_io(&r, &w);
    
    {
      time_t now;
      now = time(NULL);
      if(now >= next_poll) {
        proto_do_poll();
        next_poll = now + poll_frequency;
      }
    }
  }
}

static void text_beep() {
  xerror("BEEP!");
}

static void text_add_fd(const Connection *c, int type) {
  if(type & GUI_FD_READ) {
    FD_SET(c->fd_in, &rfds);
    if(c->fd_in >= maxfd)
      maxfd = c->fd_in + 1;
  }
  if(type & GUI_FD_WRITE) {
    FD_SET(c->fd_out, &wfds);
    if(c->fd_out >= maxfd)
      maxfd = c->fd_out + 1;
  }
}

static void text_delete_fd(const Connection *c, int type) {
  int i;

  if(type & GUI_FD_READ)
    FD_CLR(c->fd_in, &rfds);
  if(type & GUI_FD_WRITE)
    FD_CLR(c->fd_out, &wfds);

  maxfd = -1;

  for(i = 0; i < nconnections; i++) {
    int fd;

    fd = connections[i].fd_in;
    if(fd > maxfd && FD_ISSET(fd, &rfds))
      maxfd = fd;
    fd = connections[i].fd_out;
    if(fd > maxfd && FD_ISSET(fd, &wfds))
      maxfd = fd;
  }

  maxfd++;
}

static char *text_request_data(const char *prompt, int hidden) {
  char *retval;

  if(hidden) {
    char buf[1024];
    char *pass;

    snprintf(buf, 1024, "A module has requested the data item %s: ", prompt);
    pass = getpass(buf);

    retval = strdup(pass);
    while(*pass)
      *pass++ = '\0';
  }
  else {
    char buf[1024];

    printf("A module has requested the data item %s: ", prompt);
    fgets(buf, 1024, stdin);

    retval = strdup(buf);
  }
  return retval;
}

static void text_register_connections() { /* NOOP */ }
static void text_schedule_retry(time_t s) { /* NOOP */ }

GUI gui_text = {
  text_flagup,
  text_flagdown,
  text_init,
  text_display_message,
  text_event_loop,
  text_beep,
  text_add_fd,
  text_delete_fd,
  text_request_data,
  text_register_connections,
  text_schedule_retry
};
GUI *gui_text_description = &gui_text;
