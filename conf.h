#ifndef CONF_H
#define CONF_H

void conf_set_base(const char*);

int conf_read_options(void);
int conf_read_actions(void);
int conf_read_connections(void);
char *conf_read_data(const char *conn, const char *item);

unsigned long poll_frequency;

#endif /* CONF_H */
