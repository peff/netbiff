#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "imap.h"
#include "nbproto.h"

typedef struct _folder Folder;
struct _folder {
  char *name;
  int recent;
  struct _folder *next;
};

Folder *folders = NULL;

int check_folder(ImapContext *imap, Folder *f) {
  int r;

  r = imap_get_recent(imap, f->name);
  if(r < 0) {
    netbiff_send(NO, "%s", imap->errstr);
    return -1;
  }

  if(r > f->recent)
    netbiff_send(UPDATE, "%s", f->name);
  else if(r < f->recent)
    netbiff_send(RESET, "%s", f->name);
  
  f->recent = r;

  return 0;
}

int add_folder(const char *name) {
  Folder *f;

  f = malloc(sizeof(Folder));
  if(!f)
    return -1;

  f->name = strdup(name);
  if(!f->name) {
    free(f);
    return -1;
  }

  f->recent = 0;

  f->next = folders;
  folders = f;
  return 0;
}

int main(int argc, char **argv) {
  ImapContext *imap = NULL;

  if(argc < 2) {
    netbiff_send(BAD, "Insufficient arguments.");
    return 1;
  }

  if(!strcasecmp("preauth", argv[1])) {
    if(argc < 3) {
      netbiff_send(BAD, "Insufficient arguments.");
      return 1;
    }
    imap = imap_preauth(argv + 2);
    if(imap->error) {
      netbiff_send(NO, "%s", imap->errstr); 
      return 1;
    }
  }
  else if(!strcasecmp("login", argv[1])) {
    if(argc < 4) {
      netbiff_send(BAD, "Insufficient arguments.");
      return 1;
    }
    if(argc > 4) { /* do we have a password? */
      imap = imap_login(argv[2], argv[3], argv[4]);
      if(imap->error) {
        netbiff_send(NO, "%s", imap->errstr);
        return 1;
      }
    }
  }
  else {
    netbiff_send(BAD, "Unknown argument: %s", argv[1]);
    return 1;
  }

  netbiff_send(OK, "Howdy");

  while(1) {
    int cmd;
    char *arg[NETBIFF_PROTO_MAXARGS];

    cmd = netbiff_proto_next(arg);
    switch(cmd) {
      case NETBIFF_CMD_QUIT:
      case NETBIFF_CMD_EOF:
        if(imap)
          imap_destroy(imap);
        return 0;

      case NETBIFF_CMD_FOLDER:
        if(add_folder(arg[0]) < 0)
          netbiff_send(NO, "Out of memory.");
        else
          netbiff_send(OK, "%s added.", arg[0]);
        break;

      case NETBIFF_CMD_POLL:
        /* if we haven't established a connection, request the password
         * then don't poll */
        if(!imap) {
          netbiff_send(DATAREQUEST, "secret/password IMAP password for %s@%s",
              argv[3], argv[2]);
          netbiff_send(OK, "Asking for password.");
          break;
        }

        if(!arg[0]) {
          /* if no argument, poll all */
          Folder *p;
          for(p = folders; p; p = p->next)
            if(check_folder(imap, p) < 0)
              return 1;
          netbiff_send(OK, "Poll completed");
        }
        else {
          /* otherwise poll argument */
          Folder *p;
          for(p = folders; p; p = p->next)
            if(!strcmp(p->name, arg[0]))
              break;
          if(p) {
            if(check_folder(imap, p) < 0)
              return 1;
            netbiff_send(OK, "Poll completed");
          }
          else
            netbiff_send(NO, "No such folder");
        }
        break;

      case NETBIFF_CMD_DATARESPONSE:
        if(!strcasecmp(arg[0], "secret/password")) {
          imap = imap_login(argv[2], argv[3], arg[1]);
          if(imap->error) {
            netbiff_send(NO, "%s", imap->errstr);
            imap_destroy(imap);
            imap = NULL;
          }
          else {
            Folder *p;
            for(p = folders; p; p = p->next)
              if(check_folder(imap, p) < 0)
                return 1;
            netbiff_send(OK, "Data acquisition complete. Poll complete.");
          }
        }
        else {
          netbiff_send(NO, "Never heard of data item %s", arg[0]);
        }
        break;
    }
  }
}

