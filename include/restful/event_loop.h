#ifndef RESTFUL_EVENT_LOOP_H
#define RESTFUL_EVENT_LOOP_H

#include <restful/client.h>
#include <stdint.h>

#define POLL_TIMEOUT 50

typedef struct restful_loop_s {
    int fd;
    int epoll_fd;
} restful_loop_t;

int restful_init(restful_loop_t *, const char *ip, uint16_t port);
restful_loop_t *restful_new(const char *ip, uint16_t port);

void restful_dispatch(restful_loop_t *);

#endif // RESTFUL_EVENT_LOOP_H
