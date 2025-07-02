#ifndef RESTFUL_WRITE_QUEUE_H
#define RESTFUL_WRITE_QUEUE_H

#include <stddef.h>

typedef struct restful_write_node_s {
    char *data;
    size_t nbytes;
    struct restful_write_node_s *next;
} restful_write_node_t;

typedef struct restful_write_queue_s {
    restful_write_node_t *front;
    restful_write_node_t *rear;
} restful_write_queue_t;

void restful_write_queue_init(restful_write_queue_t *);
int restful_write_enqueue(restful_write_queue_t *, char *data, size_t nbytes);
void restful_write_dequeue(restful_write_queue_t *);
void restful_write_queue_free(restful_write_queue_t *);

#endif // RESTFUL_WRITE_QUEUE_H
