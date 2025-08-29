#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "./socket.h"
#include "./file.h"

void echo(int connfd)
{
    char buf[MAXLINE];

    rio_t rio;
    rio_readinitb(&rio, connfd);
    int n;

    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("server recived %d bytes\n", (int)n);
        rio_writen(connfd, buf, n);
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);
    while (1)
    {
        fprintf(stdout, "Wating client...\n");
        clientlen = sizeof(struct sockaddr_in);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Connected to (%s : %s)\n", hostname, port);
        echo(connfd);
        close(connfd);
    }
    exit(0);
}