#include <restful/client.h>
#include <restful/eventloop.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
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
    if (loop->fd == -1)
        return 1;

    const int enable = 1;
    if (setsockopt(loop->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
        close(loop->fd);
        return 1;
    }

    if (fcntl(loop->fd, F_SETFL, O_NONBLOCK) < 0) {
        close(loop->fd);
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &sin.sin_addr) != 1) {
        close(loop->fd);
        return 1;
    }

    if (bind(loop->fd, (const struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(loop->fd);
        return 1;
    }

    if (listen(loop->fd, SOMAXCONN) != 0) {
        close(loop->fd);
        return 1;
    }

    loop->epoll_fd = epoll_create1(0);
    if (loop->epoll_fd == -1) {
        close(loop->fd);
        return 1;
    }

    restful_event_t *rev = (restful_event_t *)malloc(sizeof(*rev));
    if (!rev) {
        close(loop->fd);
        close(loop->epoll_fd);
        return 1;
    }

    rev->type = RESTFUL_TYPE_SERVER;
    rev->fd = loop->fd;

    struct epoll_event ev = {.events = EPOLLIN, .data.ptr = (void *)rev};

    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, loop->fd, &ev) == -1) {
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
    if (loop) {
        if (restful_init(loop, ip, port) != 0) {
            free(loop);
            return NULL;
        }
    }
    return loop;
}

static void restful__on_read(restful_loop_t *loop, restful_event_t *event,
                             char *buf, size_t nread)
{
    // disable reading
    struct epoll_event ev = {.events = 0, .data.ptr = event};
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, event->client->fd, &ev);

    printf("read %lu bytes\n", nread);
    // printf("data: %s\n", buf);
    free(buf);
}

static int restful__add_client(restful_loop_t *loop, int fd)
{
    restful_event_t *rev = (restful_event_t *)malloc(sizeof(*rev));
    if (!rev) {
        return 1;
    }
    restful_client_t *client = restful_client_new(fd);
    if (!client) {
        free(rev);
        return 1;
    }
    rev->type = RESTFUL_TYPE_CLIENT;
    rev->client = client;

    struct epoll_event ev = {.events = EPOLLIN, .data.ptr = (void *)rev};
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        free(client);
        free(rev);
        return 1;
    }
    return 0;
}

static inline void restful__remove_client(restful_loop_t *loop,
                                          restful_event_t *event)
{
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, event->client->fd, NULL);
    close(event->client->fd);
    free(event->client);
    free(event);
}

void restful_dispatch(restful_loop_t *loop)
{
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int nfds = epoll_wait(loop->epoll_fd, events, MAX_EVENTS, POLL_TIMEOUT);
        if (nfds < 0) {
            printf("epoll_wait error\n");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            restful_event_t *rev = (restful_event_t *)events[i].data.ptr;
            if (rev->type == RESTFUL_TYPE_SERVER) {
                printf("accepting\n");
                int clientfd = accept(loop->fd, NULL, NULL);
                if (clientfd == -1) {
                    printf("accept error\n");
                    continue;
                }
                if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0) {
                    printf("fcntl error\n");
                    close(clientfd);
                    continue;
                }
                restful__add_client(loop, clientfd);
                printf("fd connected: %u\n", clientfd);
            }
            else {
                if (events[i].events & EPOLLIN) {
                    char *buf = (char *)malloc(BUFFER_SIZE);
                    if (!buf) {
                        continue;
                    }
                    printf("reading here\n");
                    ssize_t nread =
                        recv(rev->client->fd, buf, BUFFER_SIZE - 1, 0);
                    if (nread <= 0) {
                        restful__remove_client(loop, rev);
                        free(buf);
                        continue;
                    }

                    buf[nread] = '\0';
                    restful__on_read(loop, rev, buf, nread);
                }
                if (events[i].events & EPOLLOUT) {
                    // TODO: check for req in writing queue then disable flag
                }
            }
        }
    }
}
