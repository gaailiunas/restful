#include <restful/client.h>
#include <stdlib.h>

restful_client_t *restful_client_new(int fd)
{
    restful_client_t *client = (restful_client_t *)malloc(sizeof(*client));
    if (client) {
        client->fd = fd;
    }
    return client;
}
