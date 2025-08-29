#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include "./file.h"

ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            break; // EOF
        }
        nleft -= nread;
        bufp += nread; // 읽은 만큼 옮기기
    }
    return n - nleft; // 총 읽힌 바이트 수
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        nwritten = write(fd, usrbuf, nleft);
        if (nwritten < 0)
        {
            if (errno == EINTR)
            {
                nwritten = 0;
            }
            else
            {
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n - nleft;
}

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->riobuf;
}

#define MIN(a, b) ((a) > (b) ? (b) : (a))

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    while (rp->rio_cnt <= 0)
    {
        // 버퍼가 비면, 버퍼 크기만큼 채운다. 최대: sizeof 만큼, 읽은 바이트 만큼 rio_cnt
        rp->rio_cnt = read(rp->rio_fd, rp->riobuf, sizeof(rp->riobuf));

        if (rp->rio_cnt < 0) // 실패시
        {
            if (errno != EINTR)
            {
                return -1;
            }
        }
        else if (rp->rio_cnt == 0) // EOF
        {
            return 0;
        }
        else
        {
            rp->rio_bufptr = rp->riobuf;
        }
    }

    int cnt = MIN(rp->rio_cnt, n);
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++)
    {
        // rio_read를 통해 버퍼에서 1바이트 읽어옴, 비었을 땐 rio_read가 채움
        // 1바이트 씩 읽어, 개행 확인하지만 rio_read를 통해 매번 트랩하지 않음
        int rc = rio_read(rp, &c, 1);
        if (rc == 1)
        {
            *bufp++ = c;
            if (c == '\n') // 개행 나오면, 종료
            {
                n++;
                break;
            }
        }
        else if (rc == 0) // EOF
        {
            if (n == 1)
                return 0; // no data read
            else
                break;
        }
        else
        {
            return -1;
        }
    }

    *bufp = 0;
    return n - 1;
}