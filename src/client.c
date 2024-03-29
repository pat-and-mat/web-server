#include <stdio.h>
#include <stdlib.h>
#include <client.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <path_lister.h>
#include <file_sender.h>
#include <path_utils.h>
#include <frontend.h>

void client_init(struct client *client, int fd, char *ip)
{
    client->fd = fd;
    client->error = -1;
    client->status = CLIENT_STATUS_READING;
    client->request.path[0] = '\0';
    client->writer = NULL;
    strcpy(client->ip, ip);
}

void client_close(struct client *client)
{
    printf("--> Closing client %s\n", client->ip);

    close(client->fd);
    client->fd = -1;
    client->error = -1;
    client->status = CLIENT_STATUS_DONE;
    client->request.path[0] = '\0';
}

bool client_read(struct client *client)
{
    printf("--> Reading from client %s\n", client->ip);

    char buf[4096] = "\0";
    int read_count = read_line(client->fd, buf, 4096);

    char request[1024], version[1024];
    int matched = sscanf(buf, "GET %s %s", request, version);

    fix_spaces(request);

    char *pos = NULL;
    if (pos = strstr(request, "@orderby-"))
    {
        strcpy(client->request.orderby, pos + strlen("@orderby-"));
        *pos = '\0';
    }
    else
        strcpy(client->request.orderby, "name");

    sprintf(client->request.path, "%s", get_root());
    strcat(client->request.path, request);

    if (!matched)
    {
        fprintf(stderr, ERROR_COLOR "--> Method not allowed %s\n" COLOR_RESET, buf);
        client->error = 400;
    }

    if (read_count == 0)
        client->status = CLIENT_STATUS_DONE;
    else
    {
        client->status = CLIENT_STATUS_WRITING;
        if (read_count < 0)
        {
            client->error = 500;
            return false;
        }
    }
    return true;
}

void send_error(int fd, int error);

bool client_write(struct client *client)
{
    printf("--> Writing to client %s\n", client->ip);

    if (client->error > 0)
    {
        send_error(client->fd, client->error);
        client->status = CLIENT_STATUS_DONE;
        return true;
    }

    if (client->writer != NULL)
    {
        if (client->writer->write(client->writer) == WRITER_STATUS_DONE)
        {
            client->status = CLIENT_STATUS_DONE;
            free(client->writer);
            client->writer = NULL;
        }
        return true;
    }

    if (is_static(client->request.path))
    {
        client->writer = malloc(sizeof(struct static_sender));
        static_sender_init((struct static_sender *)client->writer, client->fd, client->request.path);
        return true;
    }

    if (is_dir(client->request.path))
    {
        client->writer = malloc(sizeof(struct path_lister));
        path_lister_init((struct path_lister *)client->writer, client->fd, client->request);
        return true;
    }

    if (is_file(client->request.path))
    {
        client->writer = malloc(sizeof(struct file_sender));
        file_sender_init((struct file_sender *)client->writer, client->fd, client->request.path);
        return true;
    }

    client->error = 404;
    return true;
}

void send_error(int fd, int error)
{
    char error_msg[20];
    sprintf(error_msg, "HTTP/1.0 %d\r\n\r\n", error);
    write(fd, error_msg, strlen(error_msg) * sizeof(char));
}