#include <restful/client.h>
#include <restful/logging.h>
#include <restful/write_queue.h>
#include <stdlib.h>
#include <unistd.h>

restful_client_t *restful_client_new(int fd)
{
    restful_client_t *client = (restful_client_t *)malloc(sizeof(*client));
    if (!client) {
        RESTFUL_ERR("Out of memory");
        return NULL;
    }
    client->fd = fd;
    restful_write_queue_init(&client->write_queue);
    return client;
}

int restful_client_write(restful_client_t *client, char *data, size_t nbytes)
{
    // enable POLLOUT in epoll
    return restful_write_enqueue(&client->write_queue, data, nbytes);
}

void restful_client_free(restful_client_t *client)
{
    close(client->fd);
    restful_write_queue_free(&client->write_queue);
    free(client);
}
