#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nbproto.h"

static int use_atime = 1;

typedef struct _folder Folder;
struct _folder {
  char *name;
  unsigned long last_size;
  int last_seen;
  struct _folder *next;
};

Folder *folders = NULL;

int update_folder(Folder *f);
 
void folder_destroy(Folder *f) {
  Folder *next;
  while(f) {
    if(f->name)
      free(f->name);
    next = f->next;
    free(f);
    f = next;
  }
}

int check_folder(Folder *f) {
  unsigned long size = f->last_size;

  if(update_folder(f) < 0)
    return -1;

  if(size < f->last_size && !f->last_seen)
    netbiff_send(UPDATE, "%s", f->name);
  else if(size > f->last_size || f->last_seen)
    netbiff_send(RESET, "%s", f->name);

  return 0;
}

int update_folder(Folder *f) {
  struct stat statbuf;

  if(lstat(f->name, &statbuf) < 0) {
    if(errno == ENOENT) {
      f->last_size = 0;
      f->last_seen = 1;
      return 0;
    }
    else {
      return -1;
    }
  }

  if(use_atime && (!statbuf.st_atime || !statbuf.st_mtime || 
      statbuf.st_mtime < statbuf.st_atime))
    f->last_seen = 1;
  else
    f->last_seen = 0;

  f->last_size = statbuf.st_size;

  return 0;
}

int main() {
  {
    const char* x = getenv("NETBIFFD_IGNORE_ATIME");
    if(x && atoi(x))
      use_atime = 0;
  }

  netbiff_send(OK, "Howdy.");

  while(1) {
    int cmd;
    char *arg[NETBIFF_PROTO_MAXARGS];

    cmd = netbiff_proto_next(arg);

    switch(cmd) {
      case NETBIFF_CMD_POLL:
        /* Poll for recent folder updates */
        if(!arg[0]) {
          /* Check all if none given. */
          Folder *p;

          for(p = folders; p; p = p->next)
            check_folder(p);
          netbiff_send(OK, "");
        }
        else {
          Folder *p;
          
          for(p = folders; p; p = p->next) {
            if(!strcasecmp(p->name, arg[0])) {
              if(check_folder(p) < 0) 
                netbiff_send(NO, "Error accessing %s: %s", p->name, 
                    strerror(errno));
              else 
                netbiff_send(OK, "");
            break;
            }
          }
          if(!p) 
            netbiff_send(NO, "Error accessing %s: no such folder", arg[0]);
        }
        break;

      case NETBIFF_CMD_FOLDER:
        /* Add a new folder to the list */
        {
          Folder *f;
          f = malloc(sizeof(Folder));
          if(!f) {
            netbiff_send(NO, "Out of memory.");
            continue;
          }
          f->name = strdup(arg[0]);
          if(!f->name) {
            netbiff_send(NO, "Out of memory.");
            free(f);
          }
          if(update_folder(f) < 0) {
            netbiff_send(NO, "Unable to access %s: %s", f->name,
                strerror(errno));
            free(f->name);
            free(f);
          }
          else {
            f->next = folders;
            folders = f;
            netbiff_send(OK, "");
          }
        }
        break;

      case NETBIFF_CMD_QUIT:
        return 0;

      case NETBIFF_CMD_EOF:
        return 1;
    }
  }
}
