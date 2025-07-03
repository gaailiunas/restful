#include <restful/client.h>
#include <restful/event_loop.h>
#include <restful/logging.h>
#include <restful/write_queue.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024
#define MAX_EVENTS 64

enum restful_fd_type { RESTFUL_TYPE_SERVER, RESTFUL_TYPE_CLIENT };

typedef struct {
    enum restful_fd_type type;
    union {
        int fd;
        restful_client_t *client;
    };
} restful_event_t;

int restful_init(restful_loop_t *loop, const char *ip, uint16_t port)
{
    loop->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (loop->fd == -1) {
        RESTFUL_ERR("Invalid server socket");
        return 1;
    }

    const int enable = 1;
    if (setsockopt(loop->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
        RESTFUL_ERR("Failed to set reusable addr opt to server socket");
        close(loop->fd);
        return 1;
    }

    if (fcntl(loop->fd, F_SETFL, O_NONBLOCK) < 0) {
        RESTFUL_ERR("Failed to set nonblocking opt to server socket");
        close(loop->fd);
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &sin.sin_addr) != 1) {
        RESTFUL_ERR("Invalid IP");
        close(loop->fd);
        return 1;
    }

    if (bind(loop->fd, (const struct sockaddr *)&sin, sizeof(sin)) != 0) {
        RESTFUL_ERR("Failed to bind");
        close(loop->fd);
        return 1;
    }

    if (listen(loop->fd, SOMAXCONN) != 0) {
        RESTFUL_ERR("Failed to listen");
        close(loop->fd);
        return 1;
    }

    loop->epoll_fd = epoll_create1(0);
    if (loop->epoll_fd == -1) {
        RESTFUL_ERR("Failed to create epoll socket");
        close(loop->fd);
        return 1;
    }

    restful_event_t *rev = (restful_event_t *)malloc(sizeof(*rev));
    if (!rev) {
        RESTFUL_ERR("Out of memory");
        close(loop->fd);
        close(loop->epoll_fd);
        return 1;
    }

    rev->type = RESTFUL_TYPE_SERVER;
    rev->fd = loop->fd;

    struct epoll_event ev = {.events = EPOLLIN, .data.ptr = (void *)rev};

    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, loop->fd, &ev) == -1) {
        RESTFUL_ERR("EPOLL_CTL_ADD failed");
        close(loop->fd);
        close(loop->epoll_fd);
        free(rev);
        return 1;
    }

    return 0;
}

restful_loop_t *restful_new(const char *ip, uint16_t port)
{
    restful_loop_t *loop = (restful_loop_t *)malloc(sizeof(*loop));
    if (!loop) {
        RESTFUL_ERR("Out of memory");
        return NULL;
    }
    if (restful_init(loop, ip, port) != 0) {
        RESTFUL_ERR("Failed to initialize restful");
        free(loop);
        return NULL;
    }
    return loop;
}

static void restful__on_read(restful_loop_t *loop, restful_event_t *event,
                             char *buf, size_t nread)
{
    // disable reading
    struct epoll_event ev = {.events = 0, .data.ptr = event};
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, event->client->fd, &ev);

    RESTFUL_DEBUG("read %lu bytes", nread);

    // testing
    static char resp[] = "HTTP/1.1 200 OK\r\n\r\n<h1>test</h1>";
    // char *data = (char *)malloc(sizeof(resp) - 1);
    // memcpy(data, resp, sizeof(resp) - 1);
    restful_client_write(event->client, resp, sizeof(resp) - 1);
    // ---

    ev.events = EPOLLOUT;
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, event->client->fd, &ev);

    free(buf);
}

static int restful__add_client(restful_loop_t *loop, int fd)
{
    restful_event_t *rev = (restful_event_t *)malloc(sizeof(*rev));
    if (!rev) {
        RESTFUL_ERR("Out of memory");
        return 1;
    }
    restful_client_t *client = restful_client_new(fd);
    if (!client) {
        RESTFUL_ERR("Out of memory");
        free(rev);
        return 1;
    }
    rev->type = RESTFUL_TYPE_CLIENT;
    rev->client = client;

    struct epoll_event ev = {.events = EPOLLIN, .data.ptr = (void *)rev};
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        RESTFUL_ERR("EPOLL_CTL_ADD failed");
        free(client);
        free(rev);
        return 1;
    }
    return 0;
}

static inline void restful__remove_client(restful_loop_t *loop,
                                          restful_event_t *event)
{
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, event->client->fd, NULL) ==
        -1) {
        RESTFUL_ERR("EPOLL_CTL_DEL failed");
    }
    restful_client_free(event->client);
    free(event);
}

void restful_dispatch(restful_loop_t *loop)
{
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int nfds = epoll_wait(loop->epoll_fd, events, MAX_EVENTS, POLL_TIMEOUT);
        if (nfds < 0) {
            RESTFUL_ERR("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            restful_event_t *rev = (restful_event_t *)events[i].data.ptr;
            if (rev->type == RESTFUL_TYPE_SERVER) {
                int clientfd = accept(loop->fd, NULL, NULL);
                if (clientfd == -1) {
                    RESTFUL_ERR("Failed to accept");
                    continue;
                }
                if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0) {
                    RESTFUL_ERR("Failed to set nonblockable opt to socket");
                    close(clientfd);
                    continue;
                }
                if (restful__add_client(loop, clientfd) != 0) {
                    RESTFUL_ERR("Failed to add client to epoll");
                    close(clientfd);
                    continue;
                }
                RESTFUL_DEBUG("fd connected: %u\n", clientfd);
            }
            else {
                if (events[i].events & EPOLLIN) {
                    char *buf = (char *)malloc(BUFFER_SIZE);
                    if (!buf) {
                        continue;
                    }
                    ssize_t nread =
                        recv(rev->client->fd, buf, BUFFER_SIZE - 1, 0);
                    if (nread <= 0) {
                        RESTFUL_DEBUG("nread: %ld. removing client", nread);
                        restful__remove_client(loop, rev);
                        free(buf);
                        continue;
                    }

                    buf[nread] = '\0';
                    restful__on_read(loop, rev, buf, nread);
                }
                if (events[i].events & EPOLLOUT) {
                    restful_write_node_t *write_req =
                        restful_write_dequeue(&rev->client->write_queue);
                    ssize_t nsent = send(rev->client->fd, write_req->data,
                                         write_req->nbytes, 0);
                    free(write_req);
                    if (nsent <= 0) {
                        RESTFUL_DEBUG("nsent: %ld. removing client", nsent);
                        restful__remove_client(loop, rev);
                        continue;
                    }

                    // TODO: add a callback for written requests.
                    // now it can or cant leak memory, because we don't know how
                    // its allocated

                    RESTFUL_DEBUG("sent %lu bytes", nsent);

                    if (!rev->client->write_queue.front) {
                        RESTFUL_DEBUG(
                            "Write queue is empty. Disabling EPOLLOUT");
                        struct epoll_event ev = {.events = events[i].events &
                                                           ~EPOLLOUT,
                                                 .data.ptr = (void *)rev};
                        if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD,
                                      rev->client->fd, &ev) == -1) {
                            RESTFUL_ERR("EPOLL_CTL_MOD failed");
                            restful__remove_client(loop, rev);
                            continue;
                        }
                    }
                }
            }
        }
    }
}
