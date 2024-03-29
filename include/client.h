#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_STATUS_READING 1
#define CLIENT_STATUS_WRITING 2
#define CLIENT_STATUS_DONE 3

#include <stdbool.h>
#include <writer.h>
#include <request.h>

struct client
{
    int fd;
    int status;
    int error;
    struct request request;
    char ip[20];
    struct writer *writer;
};

void client_init(struct client *client, int fd, char *ip);
void client_close(struct client *client);
bool client_read(struct client *client);
bool client_write(struct client *client);

#endif