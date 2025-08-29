#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./socket.h"
#include "./file.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *service;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    service = argv[2];

    if ((clientfd = open_clientfd(host, service)) < 0)
    {
        // fprintf(stderr, "Failed socket open: [%d]\n", clientfd);
        perror("open_clientfd");
        return -1;
    }

    rio_t rio;
    rio_readinitb(&rio, clientfd);

    char buf[MAXLINE];

    while (fgets(buf, MAXLINE, stdin) != NULL)
    {
        rio_writen(clientfd, buf, strlen(buf));
        rio_readlineb(&rio, buf, MAXLINE);
        fputs(buf, stdout);
    }
    close(clientfd);
    exit(0);
}
