#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "./socket.h"

#define LISTENQ 1024

int open_clientfd(char *hostname, char *port)
{
    int clientfd;
    struct addrinfo *listp, *p, hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;

    getaddrinfo(hostname, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }

        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
        {
            break;
        }
        close(clientfd);
    }

    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }
    else
    {
        return clientfd;
    }
}

int open_listenfd(char *port)
{
    int listenfd;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }
    if (listen(listenfd, LISTENQ) < 0)
    {
        return -1;
    }
    return listenfd;
}