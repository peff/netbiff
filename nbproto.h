#ifndef NBPROTO_H
#define NBPROTO_H

#define NETBIFF_CMD_QUIT 1
#define NETBIFF_CMD_POLL 2
#define NETBIFF_CMD_FOLDER 3
#define NETBIFF_CMD_EOF 4
#define NETBIFF_CMD_DATARESPONSE 5

#define NETBIFF_PROTO_MAXARGS 2

#define OK 0
#define NO 1
#define BAD 2
#define UPDATE 3
#define RESET 4
#define DATAREQUEST 5

int netbiff_proto_next(char **arg);
void netbiff_send(int status, char *fmt, ...);

#endif /* NBPROTO_H */
