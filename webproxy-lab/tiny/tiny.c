/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  while (strcmp(buf, "\r\n"))
  {
    rio_readlineb(rp, buf, MAXLINE);
    printf("headers: %s", buf);
  }

  return;
}

// 한개의 http 트랜잭션 처리 함수
void doit(int fd)
{
  char buf[MAXLINE];
  rio_t rio;

  // int n;
  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);

  char method[10], url[MAXLINE], version[10];
  sscanf(buf, "%s %s %s\n", method, url, version);
  printf("%s %s %s\n", method, url, version);
  if (!method || strcasecmp(method, "GET")) // get, GET
  {
    fprintf(stderr, "GET method allow: [%s]\n", method);
    return;
  }
  read_requesthdrs(&rio);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  char *port = argv[1];
  int listenfd;
  if ((listenfd = open_listenfd(port)) < 0)
  {
    perror("open_listenfd");
    exit(1);
  }

  int connfd;
  struct sockaddr_storage client_addr;
  char host[MAXLINE], serv[MAXLINE];
  while (1)
  {
    fprintf(stdout, "Wating client...\n");

    socklen_t clientaddr_len;
    connfd = accept(listenfd, (SA *)&client_addr, &clientaddr_len);
    getnameinfo((SA *)&client_addr, clientaddr_len, host, MAXLINE, serv, MAXLINE, 0);
    fprintf(stdout, "Connected (%s : %s)\n", host, serv);

    doit(connfd);
    close(connfd);
  }
}
