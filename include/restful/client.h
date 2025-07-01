#ifndef RESTFUL_CLIENT_H
#define RESTFUL_CLIENT_H

typedef struct restful_client_s {
    int fd;
} restful_client_t;

restful_client_t *restful_client_new(int fd);

#endif // RESTFUL_CLIENT_H
