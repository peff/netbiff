#ifndef PROTO_H
#define PROTO_H

#include "connection.h"
#include <sys/types.h>
#include <unistd.h>

void proto_do_io(fd_set *rfds, fd_set *wfds);
void proto_do_conn_input(Connection *c);
void proto_do_conn_output(Connection *c);
void proto_do_poll(void);
void proto_do_retries(void);

#endif /* PROTO_H */
