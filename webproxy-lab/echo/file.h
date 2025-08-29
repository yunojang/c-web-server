#include <stdio.h>

ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);

#define MAXLINE 8192

#define RIO_BUFSIZE 8192

typedef struct
{
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char riobuf[RIO_BUFSIZE];
} rio_t;

ssize_t rio_writen(int fd, void *buf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, void *usrptr, size_t max_len);