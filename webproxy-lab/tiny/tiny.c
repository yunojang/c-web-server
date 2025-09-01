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
void response_hdrs(int fd, char *mime_type, int content_len);

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mpeg"))
  {
    strcpy(filetype, "video/mpeg");
  }
  else if (strstr(filename, ".mp4"))
  {
    strcpy(filetype, "video/mp4");
  }
}

void response_line(int fd, char *version, char *status_code, char *short_msg)
{
  char buf[MAXBUF];
  sprintf(buf, "%s %s %s\r\n", version, status_code, short_msg);
  rio_writen(fd, buf, strlen(buf));
}

void response_success_line(int fd)
{
  response_line(fd, "http/1.0", "200", "OK");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  // response line
  response_success_line(fd);

  int pid;
  if ((pid = fork()) == 0)
  {
    // only child;
    // raise(SIGSTOP);
    char *empty_list[] = {filename, NULL};
    setenv("QUERY_STRING", cgiargs, 1);
    dup2(fd, STDOUT_FILENO);
    if (execve(filename, empty_list, environ) < 0)
    {
      perror("error execve");
    }
  }

  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status))
  {
    int code = WEXITSTATUS(status);
    fprintf(stderr, "error code: %d\n", code);
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  // file open - syscall
  int srcfd = open(filename, O_RDONLY, 0);

  // file read OR mmap
  void *srcp = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  close(srcfd);

  // 응답라인, 헤더 응답하기
  response_success_line(fd);
  char filetype[MAXLINE];
  get_filetype(filename, filetype);
  response_hdrs(fd, filetype, filesize);

  // fd에 파일 내용 쓰기
  rio_writen(fd, srcp, filesize);
}

static void append_snprintf(char *str, size_t maxlen, const char *fmt, ...)
{
  size_t len = strlen(str);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(str + len, maxlen, fmt, ap); // append
  // sprintf처럼 str에 fmt를 쓴다. -> 가변인자 활용
  va_end(ap);
}

// typedef struct _header
// {
//   char *title;
//   char *content;
//   struct _header *next_hdr;
// } Header;

// typedef struct _q
// {
//   void *head;
//   void *tail;
// } Q;

// static void append_hdrs(Q *q, char *title, char *content)
// {
//   Header *hdr = malloc(sizeof *hdr);
//   hdr->title = title;
//   hdr->content = content;
//   hdr->next_hdr = NULL;

//   if (q->head == NULL)
//   {
//     q->head = hdr;
//     q->tail = hdr;
//   }
//   Header *tail = (Header *)q->tail;
//   tail->next_hdr = hdr;
//   tail = hdr;
// }

// static void make_hdrs(Q *q, char *buf)
// {
//   Header *cur_hdr = q->head;
//   while (cur_hdr != NULL)
//   {
//     append_snprintf(buf, MAXLINE, "%s: %s\r\n", cur_hdr->title, cur_hdr->content);
//     cur_hdr = cur_hdr->next_hdr;
//   }
//   append_snprintf(buf, MAXLINE, "\r\n"); // last of headers
// }

void itos(int n, char *buf)
{
  snprintf(buf, MAXLINE, "%d", n);
}

void response_hdrs(int fd, char *mime_type, int content_len)
{
  char hdrs_buf[MAXLINE] = "";
  append_snprintf(hdrs_buf, MAXLINE, "Content-Type:%s\r\n", mime_type);
  append_snprintf(hdrs_buf, MAXLINE, "Content-Length: %d\r\n", content_len);
  append_snprintf(hdrs_buf, MAXLINE, "Connection: %s\r\n", "close");
  append_snprintf(hdrs_buf, MAXLINE, "\r\n");
  rio_writen(fd, hdrs_buf, strlen(hdrs_buf));
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE] = "", body[MAXBUF] = "";
  append_snprintf(body, MAXBUF, "<html><head><title>Tiny Error</title></head>\r\n");
  append_snprintf(body, MAXBUF, "<body bgcolor=\"ffffff\">\r\n");
  append_snprintf(body, MAXBUF, "<p>%s:%s</p>\r\n", errnum, shortmsg);
  append_snprintf(body, MAXBUF, "<p>%s: %s</p>\r\n", longmsg, cause);
  append_snprintf(body, MAXBUF, "<hr><em>The Tiny Web server</em></body></html>\r\n", longmsg, cause);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // version status_code status_msg
  rio_writen(fd, buf, strlen(buf));                     // response line
  response_hdrs(fd, "text/html", (int)strlen(body));    // response headers
  rio_writen(fd, body, strlen(body));                   // response body (content)
}

// uri 기반, filename, cgiargs셋 / static 여부 반환
int parse_uri(char *uri, char *filename, char *cgi_args)
{
  strcpy(cgi_args, "");

  char *query_ptr;
  if ((query_ptr = strchr(uri, '?')))
  {
    strcpy(cgi_args, query_ptr + 1);
    *query_ptr = '\0';
  }
  strcpy(filename, ".");
  strcat(filename, uri);

  if (strlen(uri) == 1 && *(uri) == '/')
  {
    strcpy(filename, "./home.html");
  }
  return !strstr(uri, "cgi-bin");
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    printf("headers: %s", buf);
    rio_readlineb(rp, buf, MAXLINE);
  }

  return;
}

// 한개의 http 트랜잭션 처리 함수
void doit(int fd)
{
  char buf[MAXLINE];
  rio_t rio;

  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);

  char method[10] = "", uri[MAXLINE] = "", version[10] = "";
  sscanf(buf, "%s %s %s\n", method, uri, version);
  printf("browser http version: %s\n", version);
  if (strcasecmp(method, "GET")) // case cmp 대소문자 무시 비교
  {
    clienterror(fd, "method", "501", "Not implemented", "Tiny does not implement this method");
    fprintf(stderr, "Method not allow [Only GET]: [%s]\n", method);
    return;
  }

  read_requesthdrs(&rio); // 빈 라인까지 헤더 읽으나, 내용은 무시 (tiny)

  char filename[MAXLINE], cgi_args[MAXLINE];
  int is_static = parse_uri(uri, filename, cgi_args); // url이 cgi를 요청하는지 아닌지

  struct stat sbuf;
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not Found", "invalid path");
    // fprintf(stderr, "404: recource\n");
    return;
  }

  if (is_static)
  {
    // file check
    if (!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IRUSR))
    {
      clienterror(fd, filename, "403", "Forbidden Error", "couldn't read the file");
      // fprintf(stderr, "forbidden\n");
      return;
    }

    serve_static(fd, filename, sbuf.st_size);
  }
  else
  {
    if (!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IXUSR))
    {
      clienterror(fd, filename, "403", "Forbidden Error", "couldn't run the cgi");
      // fprintf(stderr, "forbidden\n");
      return;
    }

    // dyan
    serve_dynamic(fd, filename, cgi_args);
  }
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
