#include "imap.h"
#include "util.h"

#include <string.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#define IMAP_DEFAULT_PORT 143
#define IMAP_DEFAULT_SSL_PORT 993

typedef void (*UntaggedCallback)(ImapContext *, char *, char *);

void handle_ignore(ImapContext *c, char *resp, char *args) {
  return;
}

void handle_status(ImapContext *c, char *resp, char *args) {
  char *p;

  if(strcasecmp(resp, "STATUS"))
    return;

  if(!args)
    goto error;

  /* Kill the final close paren */
  p = args + strlen(args) - 1;
  *p = '\0';

  /* And then find the first one */
  p = strchr(args, '(');
  if(!p)
    goto error;

  p = strchr(p, ' ');
  if(!p)
    goto error;

  p++;
  c->recent = atoi(p);
  return;

error:
  c->recent = 0;
}

static void imap_error(ImapContext *c, const char *fmt, ...) {
  va_list ap;

  c->error = 1;

  va_start(ap, fmt);
  vsnprintf(c->errstr, IMAP_BUFSIZE, fmt, ap);
  va_end(ap);
}

static void imap_serror(ImapContext *c, const char *fmt, ...) {
  va_list ap;
  char ssl_error[1024];
  char buf[IMAP_BUFSIZE];

  c->error =1;

  va_start(ap, fmt);
  vsnprintf(buf, IMAP_BUFSIZE, fmt, ap);
  va_end(ap);

  ERR_error_string(ERR_get_error(), ssl_error);
  snprintf(c->errstr, IMAP_BUFSIZE, "%s: %s", buf, ssl_error);
}

static void imap_close(ImapContext *c) {
  if(c->out) {
    BIO_puts(c->out, ". LOGOUT\r\n");
    BIO_free(c->out);
  }
  if(c->in)
    BIO_free(c->in);
  c->out = c->in = NULL;
}

static ImapContext *imap_new() {
  ImapContext *c;

  c = malloc(sizeof(ImapContext));
  if(!c)
    return NULL;

  c->in = c->out = NULL;
  c->error = 0;
  c->recent = 0;
  memset(c->buf, 0, IMAP_BUFSIZE);
  memset(c->errstr, 0, IMAP_BUFSIZE);

  return c;
}

static int imap_getline(ImapContext *c) {
  if(BIO_gets(c->in, c->buf, IMAP_BUFSIZE) < 0) {
    imap_serror(c, "read");
    imap_close(c);
    return -1;
  }
  return 0;
}

static int imap_command(ImapContext *c, UntaggedCallback ucb, 
    const char *fmt, ...) {
  va_list ap;
  BIO *b;

  va_start(ap, fmt);

  if(c->out)
    b = c->out;
  else
    b = c->in;

  BIO_puts(b, ". ");
  BIO_vprintf(b, fmt, ap);
  BIO_puts(b, "\r\n");
  BIO_flush(b);

  va_end(ap);

  while(1) {
    char *resp[3];
    int nresp;

    if(imap_getline(c) < 0)
      return -1;

    util_chomp(c->buf);
    nresp = util_split(" ", c->buf, resp, 3);
    if(nresp < 2) {
      imap_close(c);
      imap_error(c, "Confusing imap response: %s %s %s", nresp ? resp[0] : "",
          nresp > 1 ? resp[1] : "", nresp > 2 ? resp[2] : "");
      return -1;
    }
    if(!strcmp(resp[0], ".")) {
      if(!strcasecmp(resp[1], "OK"))
        return 1;
      else if(!strcasecmp(resp[1], "BAD"))
        return 0;
      else if(!strcasecmp(resp[1], "NO"))
        return 0;
      else {
        imap_close(c);
        imap_error(c, "Confusing imap response: %s %s %s", nresp ? resp[0] : "",
            nresp > 1 ? resp[1] : "", nresp > 2 ? resp[2] : "");
        return -1;
      }
    }
    else if(!strcmp(resp[0], "*")) 
      ucb(c, resp[1], resp[2]);
    else {
      imap_close(c);
      imap_error(c, "Confusing imap response: %s %s %s", nresp ? resp[0] : "",
          nresp > 1 ? resp[1] : "", nresp > 2 ? resp[2] : "");
      return -1;
    }
  }
}

static int imap_open_io(ImapContext *c, int use_ssl, int rfd, int wfd) {
  SSL_library_init();
  ERR_load_crypto_strings();
  ERR_load_SSL_strings();

  if(use_ssl) {
    SSL_CTX *ssl_ctx;
    BIO *bio;

    ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if(!ssl_ctx) {
      imap_serror(c, "Unable to initialize SSL context:");
      return -1;
    }

    c->in = BIO_new_socket(rfd, BIO_CLOSE);
    if(!c->in) {
      imap_serror(c, "BIO_new_socket");
      return -1;
    }

    bio = BIO_new_ssl(ssl_ctx, 1);
    if(!bio) {
      imap_serror(c, "BIO_new_ssl");
      return -1;
    }
    c->in = BIO_push(bio, c->in);

    bio = BIO_new(BIO_f_buffer());
    if(!bio) {
      imap_serror(c, "BIO_new_buffer");
      return -1;
    }
    c->in = BIO_push(bio, c->in);
  }
  else {
    c->in = BIO_new_socket(rfd, BIO_CLOSE);
    c->in = BIO_push(BIO_new(BIO_f_buffer()), c->in);
    if(wfd >= 0) {
      c->out = BIO_new_socket(wfd, BIO_CLOSE);
      c->out = BIO_push(BIO_new(BIO_f_buffer()), c->out);
    }
  }

  return 0;
}

ImapContext *imap_preauth(char * const cmd[]) {
  ImapContext *c;
  pid_t pid;
  int read, write;
  char *resp[3];
  int nresp;

  c = imap_new();
  if(!c)
    return NULL;

  pid = util_pipeto(cmd, &read, &write);
  if(!pid) {
    imap_error(c, "%s", strerror(errno));
    return c;
  }

  if(imap_open_io(c, 0, read, write) < 0)
    return c;

  if(imap_getline(c) < 0)
    return c;
  util_chomp(c->buf);
  nresp = util_split(" ", c->buf, resp, 3);
  if(nresp < 2 || strcmp(resp[0], "*") || strcasecmp(resp[1], "PREAUTH")) {
    imap_close(c);
    imap_error(c, "Unexpected preauth greeting: %s %s %s", 
        nresp ? resp[0] : "", nresp > 1 ? resp[1] : "", 
        nresp > 2 ? resp[2] : "");
    return c;
  }

  return c;
}

static int parse_host(const char *s, char **ret_host, int *use_ssl, 
    unsigned short *port) {
  char *port_string;
  char *ssl_string;
  char *host;

  *ret_host = host = strdup(s);
  if(!host)
    return -1;
  
  port_string = strchr(host, ':');
  if(port_string)
    *port_string++ = '\0';

  ssl_string = strchr(host, '/');
  if(ssl_string && !strcasecmp(ssl_string, "/ssl")) {
    *ssl_string = '\0';
    *use_ssl = 1;
  }
  else
    *use_ssl = 0;

  /* !! we should use one port and support STARTTLS */
  if(!port_string)
    *port = *use_ssl ? IMAP_DEFAULT_SSL_PORT : IMAP_DEFAULT_PORT;
  else
    *port = atoi(port_string);

  return 0;
}

ImapContext *imap_login(const char *host, const char *login, const char *pass) {
  ImapContext *c;

  int use_ssl;
  unsigned short port;
  char *hostname;

  int fd;

  char *resp[3];
  int nresp;

  c = imap_new();
  if(!c)
    return NULL;

  if(parse_host(host, &hostname, &use_ssl, &port) < 0) {
    imap_destroy(c);
    return NULL;
  }

  fd = util_iconnect(hostname, port);
  if(fd < 0) {
    if(fd < -1) {
#ifdef HAVE_HSTRERROR
      imap_error(c, "Unable to resolve %s: %s", host, hstrerror(h_errno));
#else
      imap_error(c, "Unable to resolve %s", host);
#endif /* HAVE_STRERROR */
      return c;
    }
    else {
      imap_error(c, "Unable to connect to %s/%hu: %s", host, port, 
          strerror(errno));
      return c;
    }
  }

  if(imap_open_io(c, use_ssl, fd, -1) < 0)
    return c;

  if(imap_getline(c) < 0)
    return c;
  util_chomp(c->buf);
  nresp = util_split(" ", c->buf, resp, 3);
  if(nresp < 2 || strcmp(resp[0], "*") || strcasecmp(resp[1], "OK")) {
    imap_close(c);
    imap_error(c, "Unexpected IMAP greeting: %s %s %s", 
        nresp ? resp[0] : "", nresp > 1 ? resp[1] : "", 
        nresp > 2 ? resp[2] : "");
    return c;
  }
  
  switch(imap_command(c, handle_ignore, "LOGIN %s \"%s\"", login, pass)) {
    case -1:
      return c;
    case 0:
      imap_close(c);
      imap_error(c, "LOGIN command failed");
      return c;
    default:
      return c;
  }
}

void imap_destroy(ImapContext *c) {
  imap_close(c);
  free(c);
}

int imap_get_recent(ImapContext *c, const char *folder) {
  c->error = 0;
  switch(imap_command(c, handle_status, "STATUS %s (UNSEEN)", folder)) {
    case -1:
      return -1;
    case 0:
      return 0;
    default:
      return c->recent;
  }
}

