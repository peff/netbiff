#ifndef IMAP_H
#define IMAP_H

#define IMAP_BUFSIZE 4096

#ifdef USE_SSL
#include <openssl/bio.h>
#else
#include <stdio.h>
#endif /* USE_SSL */


typedef struct _ImapContext ImapContext;
struct _ImapContext {
#ifdef USE_SSL
  BIO *in;
  BIO *out;
#else
  FILE *in;
  FILE *out;
#endif /* USE_SSL */
  char buf[IMAP_BUFSIZE];
  int error;
  char errstr[IMAP_BUFSIZE];
  int recent;
};

ImapContext *imap_preauth(char * const cmd[]);
ImapContext *imap_login(const char *host, const char *login, const char *pass);

int imap_get_recent(ImapContext *c, const char *folder);

void imap_destroy(ImapContext *c);

#endif /* IMAP_H */
