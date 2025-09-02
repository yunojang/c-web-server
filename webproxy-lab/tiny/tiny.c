/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

enum Method
{
  GET,
  HEAD,
  // POST,
  NOT_ALLOW,
};

enum ResourceKind
{
  RES_STATIC,
  RES_CGI,
  RES_NOTFOUND,
  RES_FORBIDDEN,
};

typedef struct _request
{
  enum Method method;
  char *uri;
} Request;

typedef struct
{
  enum ResourceKind kind;
  struct stat st;
  char *qs;
  char *filename;
} RouteInfo;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg, int head);
void response_hdrs(int fd, char *mime_type, int content_len);

// utils

static void append_snprintf(char *str, size_t maxlen, const char *fmt, ...)
{
  size_t len = strlen(str);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(str + len, maxlen, fmt, ap); // append
  // sprintf처럼 str에 fmt를 쓴다. -> 가변인자 활용
  va_end(ap);
}

// ---

// response utils
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

void response_hdrs(int fd, char *mime_type, int content_len)
{
  char hdrs_buf[MAXLINE] = "";
  append_snprintf(hdrs_buf, MAXLINE, "Content-Type:%s\r\n", mime_type);
  append_snprintf(hdrs_buf, MAXLINE, "Content-Length: %d\r\n", content_len);
  append_snprintf(hdrs_buf, MAXLINE, "Connection: %s\r\n", "close");
  append_snprintf(hdrs_buf, MAXLINE, "\r\n");
  rio_writen(fd, hdrs_buf, strlen(hdrs_buf));
}

void response_success_line(int fd)
{
  response_line(fd, "http/1.0", "200", "OK");
}
// ---

// handlers
void serve_static(int fd, char *filename, int filesize, int only_head)
{
  int srcfd = open(filename, O_RDONLY, 0);
  void *srcp = malloc(filesize);
  // read to memory
  rio_readn(srcfd, srcp, filesize);
  // map to memory
  // void *srcp = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  close(srcfd);

  response_success_line(fd); // response line

  char filetype[MAXLINE];
  get_filetype(filename, filetype);
  response_hdrs(fd, filetype, filesize); // response hdrs

  if (!only_head)
  {
    rio_writen(fd, srcp, filesize); // response content
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head)
{
  response_success_line(fd); // response line
  int pid;
  if ((pid = fork()) == 0)
  {
    char *empty_args[] = {filename, NULL};
    setenv("IS_HEAD", is_head ? "1" : "0", 1);
    setenv("QUERY_STRING", cgiargs, 1);
    dup2(fd, STDOUT_FILENO);
    if (execve(filename, empty_args, environ) < 0)
    {
      perror("error execve");
    }
  }
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status))
  {
    fprintf(stderr, "exit code: %d\n", WEXITSTATUS(status));
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, int only_head)
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
  if (!only_head)
  {
    rio_writen(fd, body, strlen(body)); // response body (content)
  }
}
// ---

// request utils
int parse_uri(char *uri, char *filename, char *cgi_args)
{
  // uri -> [filename, cgiargs] & static 여부 반환
  strcpy(cgi_args, "");

  char *query_ptr;
  if ((query_ptr = strchr(uri, '?')))
  {
    strcpy(cgi_args, query_ptr + 1);
    *query_ptr = '\0'; // uri -> filename 까지
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
// ---

// choose handlers
static void dispatch(const Request *req, RouteInfo *route, int fd)
{
  int is_head = req->method == HEAD;
  if (req->method == NOT_ALLOW)
  {
    clienterror(fd, "method", "501", "Not implemented",
                "Tiny does not implement this method", is_head);
    return;
  }
  if (route->kind == RES_NOTFOUND)
  {
    clienterror(fd, route->filename, "404", "Not Found", "",
                is_head);
    return;
  }
  if (route->kind == RES_FORBIDDEN)
  {
    clienterror(fd, route->filename, "403", "Forbidden",
                route->kind == RES_STATIC ? "cannot read" : "cannot run", is_head);
    return;
  }

  if (route->kind == RES_STATIC)
  {
    serve_static(fd, route->filename, route->st.st_size, is_head);
  }
  else
  {
    serve_dynamic(fd, route->filename, route->qs, is_head);
  }
}

// static or cgi, file chk
static void route_request(const Request *req, RouteInfo *out)
{
  char filename[MAXLINE], cgi_args[MAXLINE];
  int is_static = parse_uri(req->uri, filename, cgi_args);
  out->kind = is_static ? RES_STATIC : RES_CGI;
  out->filename = filename;
  out->qs = cgi_args;

  struct stat sbuf;
  if (stat(filename, &sbuf) < 0)
  {
    out->kind = RES_NOTFOUND;
    return;
  }
  out->st = sbuf;

  if (!S_ISREG(sbuf.st_mode))
  {
    out->kind = RES_FORBIDDEN;
  }
  if (is_static && !(sbuf.st_mode & S_IRUSR))
  {
    out->kind = RES_FORBIDDEN;
  }
  if (!is_static && !(sbuf.st_mode & S_IXUSR))
  {
    out->kind = RES_FORBIDDEN;
  }
}

// 한개의 http 트랜잭션 처리 함수
void doit(int fd)
{
  char buf[MAXLINE];
  rio_t rio;
  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf); // request line

  char method[10] = "", uri[MAXLINE] = "", version[10] = "";
  sscanf(buf, "%s %s %s\n", method, uri, version);
  printf("browser http version: %s\n", version);
  read_requesthdrs(&rio); // 빈 라인까지 헤더 읽으나, 내용은 무시 (tiny)

  Request req;
  req.uri = uri;
  int is_get = strcasecmp(method, "GET") == 0; // case cmp 대소문자 무시 비교
  int is_head = strcasecmp(method, "HEAD") == 0;
  if (is_get)
  {
    req.method = GET;
  }
  else if (is_head)
  {
    req.method = HEAD;
  }
  else
  {
    req.method = NOT_ALLOW;
  }

  RouteInfo ri;
  route_request(&req, &ri);
  dispatch(&req, &ri, fd);
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
