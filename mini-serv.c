#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

typedef struct s_client {
    int id;
    int fd;
    char *buf;
    struct s_client *next;
} t_client;

t_client *clients = NULL;
fd_set active_fds, read_fds;
int next_id = 0;

void fatal_error() {
    write(2, "Fatal error\n", 12);
    exit(1);
}

int extract_message(char **buf, char **msg) {
    char *newbuf;
    int i = 0;

    *msg = 0;
    if (*buf == 0)
        return 0;
    while ((*buf)[i]) {
        if ((*buf)[i] == '\n') {
            newbuf = calloc(1, strlen(*buf + i + 1) + 1);
            if (!newbuf)
                return -1;
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = '\0';
            *buf = newbuf;
            return 1;
        }
        i++;
    }
    return 0;
}

char *str_join(char *buf, char *add) {
    char *newbuf;
    int len = buf ? strlen(buf) : 0;

    newbuf = malloc(len + strlen(add) + 1);
    if (!newbuf)
        return NULL;
    newbuf[0] = '\0';
    if (buf)
        strcat(newbuf, buf);
    strcat(newbuf, add);
    free(buf);
    return newbuf;
}

t_client *add_client(int fd) {
    t_client *new = calloc(1, sizeof(t_client));
    if (!new)
        fatal_error();
    new->id = next_id++;
    new->fd = fd;
    new->next = clients;
    clients = new;
    return new;
}

void remove_client(int fd) {
    t_client **cur = &clients;
    while (*cur) {
        if ((*cur)->fd == fd) {
            t_client *to_remove = *cur;
            *cur = (*cur)->next;
            if (to_remove->buf)
                free(to_remove->buf);
            close(to_remove->fd);
            free(to_remove);
            FD_CLR(fd, &active_fds);
            return;
        }
        cur = &(*cur)->next;
    }
}

t_client *find_client(int fd) {
    t_client *cur = clients;
    while (cur) {
        if (cur->fd == fd)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void send_all(int sender_fd, const char *msg) {
    t_client *cur = clients;
    while (cur) {
        if (cur->fd != sender_fd)
            send(cur->fd, msg, strlen(msg), 0);
        cur = cur->next;
    }
}

int main(int argc, char **argv) {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[65536], tmp[65536];
    
    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    FD_ZERO(&active_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatal_error();

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        fatal_error();
    if (listen(sockfd, SOMAXCONN) < 0)
        fatal_error();

    FD_SET(sockfd, &active_fds);
    int max_fd = sockfd;

    while (1) {
        read_fds = active_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            fatal_error();
        for (int fd = 0; fd <= max_fd; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == sockfd) {
                    len = sizeof(cliaddr);
                    connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
                    if (connfd < 0)
                        fatal_error();
                    t_client *new_client = add_client(connfd);
                    FD_SET(connfd, &active_fds);
                    if (connfd > max_fd)
                        max_fd = connfd;

                    sprintf(tmp, "server: client %d just arrived\n", new_client->id);
                    send_all(connfd, tmp);
                } else {
                    t_client *client = find_client(fd);
                    int ret = recv(fd, buffer, sizeof(buffer) - 1, 0);
                    if (ret <= 0) {
                        sprintf(tmp, "server: client %d just left\n", client->id);
                        send_all(fd, tmp);
                        remove_client(fd);
                        break;
                    } else {
                        buffer[ret] = '\0';
                        client->buf = str_join(client->buf, buffer);
                        char *msg;
                        while (extract_message(&client->buf, &msg)) {
                            sprintf(tmp, "client %d: %s", client->id, msg);
                            send_all(fd, tmp);
                            free(msg);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
