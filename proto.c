#include "proto.h"
#include "buffer.h"
#include "connection.h"
#include "xlib.h"
#include "gui.h"
#include "conf.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static void proto_init_conn(Connection *c) {
  xerror("%s: Initializing connection", c->name);
  if(xpopen2(c->command, &c->fd_in, &c->fd_out) < 0)
    return;
  c->state = CSTATE_WAIT_HELLO;
  gui.add_fd(c, GUI_FD_READ);
}

static void proto_poll_conn(Connection *c) {
  static const char pmsg[] = "POLL\n";
  buffer_put(&c->bout, pmsg, sizeof(pmsg) - 1);
  gui.add_fd(c, GUI_FD_WRITE);
  c->pending_commands++;
}

static void proto_add_next_folder(Connection *c) {
  static const char fmsg[] = "FOLDER ";
  const char *f;

  if(c->folders_added == c->nfolders) {
    c->state = CSTATE_NORMAL;
    proto_poll_conn(c);
    return;
  }

  f = c->folders[c->folders_added].name;
  xerror("%s: Adding folder %s", c->name, f);
  buffer_put(&c->bout, fmsg, sizeof(fmsg) - 1);
  buffer_put(&c->bout, f, strlen(f));
  buffer_put(&c->bout, "\n", 1);
  gui.add_fd(c, GUI_FD_WRITE);
}

static void proto_process_untagged(Connection *c, char *s) {
  char *resp[2];
  int nresp;

  nresp = xsplit(" ", s, resp, 2);
  if(!strcasecmp(resp[0], "UPDATE")) {
    if(!resp[1])
      return;
    connection_do_update(c, resp[1]);
  }
  else if(!strcasecmp(resp[0], "RESET")) {
    if(!resp[1])
        return;
    connection_do_reset(c, resp[1]);
  }
  else if(!strcasecmp(resp[0], "DATAREQUEST")) {
    char *args[2];
    int nargs;
    char *data;
    static const char respmsg[] = "DATARESPONSE ";

    /* ignore datarequests unless we're out of the "initial" states */
    if(c->state != CSTATE_NORMAL)
      return;

    if(!resp[1])
      return;
    nargs = xsplit(" ", resp[1], args, 2);
    if(nargs < 2)
      return;

    data = conf_read_data(c->name, args[0]);
    if(!data)
      data = gui.request_data(args[1], 1);
    if(!data)
      return;

    buffer_put(&c->bout, respmsg, sizeof(respmsg)-1);
    buffer_put(&c->bout, args[0], strlen(args[0]));
    buffer_put(&c->bout, " ", 1);
    buffer_put(&c->bout, data, strlen(data));
    buffer_put(&c->bout, "\n", 1);

    free(data);
    
    gui.add_fd(c, GUI_FD_WRITE);
    c->pending_commands++;
  }
}

static void proto_process_line(Connection *c, char *s) {
  char *resp[2];
  int nresp;

  xchomp(s);
  if(!*s)
    return;
  nresp = xsplit(" ", s, resp, 2);

  if(!strcasecmp(resp[0], "OK")) {
    switch(c->state) {
      case CSTATE_WAIT_HELLO:
        c->penalty = 1;
        c->state = CSTATE_ADDING_FOLDERS;
        proto_add_next_folder(c);
        break;
      case CSTATE_ADDING_FOLDERS:
        c->folders_added++;
        proto_add_next_folder(c);
        break;
      case CSTATE_NORMAL:
        if(c->pending_commands > 0)
          c->pending_commands--;
        else
          xerror("%s: unexpected OK response", c->name);
        break;
    }
  }

  else if(!strcasecmp(resp[0], "NO")) {
    xerror("%s: %s", c->name, resp[1] ? resp[1] : "Unknown Error");
    switch(c->state) {
      case CSTATE_WAIT_HELLO:
        connection_close(c);
        break;
      case CSTATE_ADDING_FOLDERS:
        connection_close(c);
        break;
      case CSTATE_NORMAL:
        if(c->pending_commands > 0)
          c->pending_commands--;
        else
          xerror("%s: unexpected NO response", c->name);
        break;
    }
  }

  else if(!strcasecmp(resp[0], "BAD")) {
    xerror("%s reports protocol error from our end: %s", 
        c->name, 
        resp[1] ? resp[1] : "Unknown Error");
    connection_close(c);
  }

  else if(!strcasecmp(resp[0], "*")) {
    if(nresp > 1) 
      proto_process_untagged(c, resp[1]);
  }

  else {
    xerror("%s sent us gibberish: %s", c->name, resp[0]);
    connection_close(c);
  }
}

void proto_do_conn_input(Connection *c) {
  char buf[4096];
  int result;
  char *p;

  result = read(c->fd_in, buf, sizeof(buf));
  if(result < 0) {
    xerror("%s: read error: %s", c->name, xsyserr());
    connection_close(c);
    return;
  }
  else if(result == 0) {
    xerror("%s: Hangup Detected", c->name);
    connection_close(c);
    return;
  }
  buffer_put(&c->bin, buf, result);

  while((p = buffer_get_line(&c->bin))) {
    proto_process_line(c, p);
    free(p);
  }
}

void proto_do_conn_output(Connection *c) {
  char *p;
  int len;

  len = buffer_get(&c->bout, &p);
  if(len) {
    int result;
    result = write(c->fd_out, p, len);
    if(result < 0) {
      xerror("%s: write: %s", c->name, xsyserr);
      connection_close(c);
    }
    else if(result < len) 
      buffer_put(&c->bout, p + result, len - result);
    free(p);
  }
  if(!buffer_has_data(&c->bout))
    gui.delete_fd(c, GUI_FD_WRITE);
}

void proto_do_io(fd_set *rfds, fd_set *wfds) {
  int i;

  for(i = 0; i < nconnections; i++) {
    if(FD_ISSET(connections[i].fd_in, rfds))
      proto_do_conn_input(connections+i);
    if(FD_ISSET(connections[i].fd_out, wfds))
      proto_do_conn_output(connections+i);
  }
}

void proto_do_poll() {
  int i;

  for(i = 0; i < nconnections; i++)
    if(connections[i].state == CSTATE_NORMAL 
        && !connections[i].pending_commands)
      proto_poll_conn(connections+i);
}

void proto_do_retries() {
  int i;
  time_t now;

  now = time(0);

  for(i = 0; i < nconnections; i++) {
    if(connections[i].state == CSTATE_PENALTY_BOX) {
      if(connections[i].retry_time <= now) {
        proto_init_conn(connections+i);
      }
    }
  }
}
