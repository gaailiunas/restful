#ifndef RESTFUL_CLIENT_H
#define RESTFUL_CLIENT_H

#include <restful/write_queue.h>

typedef struct restful_client_s {
    int fd;
    restful_write_queue_t write_queue;
} restful_client_t;

restful_client_t *restful_client_new(int fd);
int restful_client_write(restful_client_t *client, char *data, size_t nbytes);
void restful_client_free(restful_client_t *);

#endif // RESTFUL_CLIENT_H
