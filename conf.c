#include "conf.h"
#include "connection.h"
#include "xlib.h"
#include "defaults.h"
#include "dir.h"

#include <stdio.h>
#include <errno.h>

unsigned long poll_frequency = DEFAULT_POLL_FREQUENCY;

static const char *conf_base = "/";

void conf_set_base(const char* s) {
  conf_base = xstrdup(s);
}

static void parse_error(const char* s1, const char *s2, const char *s3) {
  xerror("%s%s%s%s%s",
      s1,
      (s2 ? ": " : ""), (s2 ? s2 : ""),
      (s3 ? ": " : ""), (s3 ? s3 : "")
  );
}

static char *slurp_data_string(FILE *f) {
  int used = 0;
  int avail = 64;
  char *ret;

  ret = xmalloc(avail);

  for(;;) {
    int ch;

    if(used == avail) {
      avail *= 2;
      ret = xrealloc(ret, avail);
    }

    ch = getc(f);
    if(ch == EOF) {
      ret[used] = '\0';
      return ret;
    }

    ret[used++] = ch;
  }
}

static char *read_data_string(const char *name) {
  FILE *f;
  char *ret;

  f = fopen(name, "r");
  if(!f)
    return NULL;

  ret = slurp_data_string(f);

  if(ferror(f)) {
    fclose(f);
    return NULL;
  }

  fclose(f);
  return ret;
}

static int conf_add_update(Connection *c, const char *s) {
  const Action *a;

  if(c->nupdates == MAX_CONNECTION_UPDATES) {
    parse_error(c->name,"too many update actions",0);
    return -1;
  }

  a = action_find(s);
  if(!a) {
    parse_error(c->name,"no such action",s);
    return -1;
  }

  c->updates[c->nupdates++] = a;
  return 0;
}

static int conf_add_reset(Connection *c, const char *s) {
  const Action *a;

  if(c->nresets == MAX_CONNECTION_RESETS) {
    parse_error(c->name,"too many reset actions",0);
    return -1;
  }

  a = action_find(s);
  if(!a) {
    parse_error(c->name,"no such action",s);
    return -1;
  }

  c->resets[c->nresets++] = a;
  return 0;
}

static int conf_add_folder(Connection *c, const char *s) {
  if(c->nfolders == MAX_CONNECTION_FOLDERS) {
    parse_error(c->name,"too many folders",0);
    return -1;
  }

  c->folders[c->nfolders].name = xstrdup(s);
  c->folders[c->nfolders].biffed = 0;
  c->nfolders++;
  return 0;
}

static int conf_add_action(const char *rel, const char *full) {
  char *cmd;

  cmd = read_data_string(full);
  if(!cmd) {
    parse_error(full,"unable to read data",xsyserr());
    return -1;
  }

  action_add(ActionShell, rel, cmd);

  free(cmd);
  return 0;
}

static int read_data_list(const char* name, 
    int (*cb)(Connection *,const char *), Connection *conn) {
  FILE *f;
  char buf[4096];

  f = fopen(name, "r");
  if(!f)
    return -1;

  while(fgets(buf, sizeof(buf), f)) {
    xchomp(buf);
    if(cb(conn, buf) < 0) {
      fclose(f);
      return -1;
    }
  }

  fclose(f);
  return 0;
}

static int conf_add_connection(const char *rel, const char *full) {
  char buf[PATH_MAX];
  Connection *c;
 
  c = connections + nconnections;
  connection_init(c);
  c->name = xstrdup(rel);

  make_path(buf, full, "command");
  c->command = read_data_string(buf);
  if(!c->command) {
    parse_error(buf,"cannot read command data",xsyserr());
    return -1;
  }

  make_path(buf, full, "update");
  if(read_data_list(buf, conf_add_update, c) < 0) {
    if (errno != ENOENT) {
      parse_error(buf,"unable to read update data",xsyserr());
      return -1;
    }
    conf_add_update(c, "image");
    conf_add_update(c, "beep");
  }
  
  make_path(buf, full, "reset");
  if(read_data_list(buf, conf_add_reset, c) < 0) {
    if (errno != ENOENT) {
      parse_error(buf,"unable to read reset data",xsyserr());
      return -1;
    }
    conf_add_reset(c, "image-reset");
  }
  
  make_path(buf, full, "folders");
  if(read_data_list(buf, conf_add_folder, c) < 0) {
    if (errno != ENOENT) {
      parse_error(buf,"unable to read folder data",xsyserr());
      return -1;
    }
    conf_add_folder(c, "INBOX");
  }

  make_path(buf, full, "secret");
  if(!dir_is_protected(buf)) {
    parse_error(buf,"\"secret\" directory has loose permissions",0);
    return -1;
  }

  nconnections++;
  return 0;
}

static int 
conf_dir_foreach(const char *s, int (*cb)(const char *, const char *)) {
  int e;
  e = dir_foreach(s, cb);
  if(e < 0) {
    parse_error(s,"unable to open directory",xsyserr());
    return -1;
  }
  else if(e > 0)
    return -1;
  else
    return 0;
}

int conf_read_actions(void) {
  char buf[PATH_MAX];
  make_path(buf, conf_base, "actions");
  return conf_dir_foreach(buf, conf_add_action);
}

int conf_read_connections(void) {
  char buf[PATH_MAX];
  make_path(buf, conf_base, "connections");
  return conf_dir_foreach(buf, conf_add_connection);
}

int conf_read_options(void) {
  char buf[PATH_MAX];
  char *data;

  make_path(buf, conf_base, "frequency");
  data = read_data_string(buf);
  if(data) {
    poll_frequency = atoi(data);
    free(data);
  }

  return 0;
}

static void strip_newlines(char *in) {
  char *out;

  out = in;
  for(;;) {
    switch(*in) {
      case '\0':
        *out = *in;
        return;
      case '\n':
        in++;
        break;
      default:
        *out++ = *in++;
        break;
    }
  }
}

char *conf_read_data(const char *tag, const char *item) {
  char buf[PATH_MAX];
  char *d;

  make_path(buf, conf_base, "connections");
  make_path(buf, buf, tag);
  make_path(buf, buf, item);

  d = read_data_string(buf);
  if(d)
    strip_newlines(d);

  return d;
}
