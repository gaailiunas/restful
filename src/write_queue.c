#include <restful/logging.h>
#include <restful/write_queue.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>

void restful_write_queue_init(restful_write_queue_t *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

int restful_write_enqueue(restful_write_queue_t *queue, char *data,
                          size_t nbytes)
{
    restful_write_node_t *node = (restful_write_node_t *)malloc(sizeof(*node));
    if (!node) {
        RESTFUL_ERR("Out of memory");
        return 1;
    }

    node->data = data;
    node->nbytes = nbytes;
    node->next = NULL;

    if (!queue->front) {
        queue->front = node;
        queue->rear = node;
    }
    else {
        queue->rear->next = node;
        queue->rear = node;
    }

    return 0;
}

void restful_write_dequeue(restful_write_queue_t *queue)
{
    if (!queue->front) {
        RESTFUL_DEBUG("No queue front node");
        return;
    }
    restful_write_node_t *tmp = queue->front;
    queue->front = queue->front->next;
    if (!queue->front)
        queue->rear = NULL;
    free(tmp);
}

void restful_write_queue_free(restful_write_queue_t *queue)
{
    restful_write_node_t *node = queue->front;
    while (node) {
        restful_write_node_t *tmp = node;
        node = node->next;
        free(tmp);
    }
    queue->front = NULL;
    queue->rear = NULL;
}
