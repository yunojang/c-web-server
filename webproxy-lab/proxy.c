#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

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

static void get_hdr_val(const char *hdrs, const char *hdr_name, char *out_val)
{
  const char *p = strcasestr(hdrs, hdr_name);
  if (!p)
  {
    return;
  }

  p += strlen(hdr_name);
  while (*p == ' ' || *p == '\t')
  {
    p++;
  }

  int i;
  for (i = 0; i < MAXLINE - 1; i++)
  {
    if (!*p || *p == '\r')
    {
      break;
    }
    out_val[i] = *p++;
  }
  out_val[i] = '\0';
}

static void parse_url(const char *url, char *hostname, char *port, char *path)
{
  const char *p = strncmp(url, "http://", 7) == 0 ? url + 7 : url; // skip http://
  char buf[MAXLINE];
  strcpy(buf, p);

  char *slash = strchr(buf, '/');
  strcpy(path, slash ? slash : "/");
  if (slash)
    *slash = '\0';

  char *colon = strchr(buf, ':');
  stpcpy(port, colon ? colon + 1 : "80");
  if (colon)
    *colon = '\0';
  strcpy(hostname, buf);
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

// ---

// handler
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

// --

// request
void read_requesthdrs(rio_t *rp, char *out)
{
  char line[MAXLINE];
  out[0] = '\0'; // 초기화
  while (rio_readlineb(rp, line, MAXLINE) > 0)
  {
    if (strcmp(line, "\r\n") == 0)
    {
      break;
    }
    strcat(out, line);
  }
  return;
}

void proxy_response(int origin_server_fd, int client_fd)
{
  rio_t rio_s;
  rio_readinitb(&rio_s, origin_server_fd);
  char buf[MAXLINE] = "";
  size_t content_len = 0;

  // 헤더 읽고 전송
  while (rio_readlineb(&rio_s, buf, MAXLINE) != 0)
  {
    rio_writen(client_fd, buf, strlen(buf));

    if (strstr(buf, "Content-Length:") != NULL)
    {
      sscanf(buf, "Content-Length: %zu", &content_len);
    }

    if (strcmp(buf, "\r\n") == 0)
    {
      break;
    }
  }

  printf("response content_len: %zu\n", content_len);

  if (content_len > 0)
  {
    char content_buf[content_len];
    rio_readnb(&rio_s, content_buf, content_len);
    rio_writen(client_fd, content_buf, content_len);
  }
}

void request_hdrs(int fd, const char *req_hdrs, const char *host_name)
{
  char hdrs_buf[MAXBUF] = "";
  append_snprintf(hdrs_buf, MAXBUF, "%s", req_hdrs);
  if (strcasestr(hdrs_buf, "host") == NULL)
  {
    append_snprintf(hdrs_buf, MAXBUF, "Host: %s\r\n", host_name);
  }
  append_snprintf(hdrs_buf, MAXBUF, "User-Agent: %s", user_agent_hdr);
  append_snprintf(hdrs_buf, MAXBUF, "Connection: close\r\n");
  append_snprintf(hdrs_buf, MAXBUF, "Proxy-Connection: close\r\n");
  append_snprintf(hdrs_buf, MAXBUF, "\r\n");
  rio_writen(fd, hdrs_buf, strlen(hdrs_buf));
}

int request(int fd, char *hostname, char *port, char *path, char *hdrs)
{
  int clientfd;
  if ((clientfd = open_clientfd(hostname, port)) < 0)
  {
    perror("open_clientfd");
    return -1;
  }

  char buf[MAXLINE];
  sprintf(buf, "GET %s http/1.0\r\n", path);
  rio_writen(clientfd, buf, strlen(buf));
  request_hdrs(clientfd, hdrs, hostname);

  proxy_response(clientfd, fd);
  return 0;
}

void request_proxy(int fd)
{
  char buf[MAXBUF];
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];
  rio_t rio;
  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXBUF);
  printf("%s", buf);
  sscanf(buf, "%s %s %s\r\n", method, url, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, "[proxy]method", "501", "Not implement", "", 0);
    fprintf(stderr, "invalid method [%s]\n", method);
    return;
  }

  char hdrs[MAXBUF];
  read_requesthdrs(&rio, hdrs);
  printf("Request-Headers:\n");
  printf("%s", hdrs);

  char host_val[MAXLINE];
  get_hdr_val(hdrs, "Host:", host_val);

  int is_abs = strncmp(url, "http://", 7) == 0;
  char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
  if (is_abs)
  {
    parse_url(url, hostname, port, path);
  }
  else
  {
    parse_url(host_val, hostname, port, path);
  }
  printf("Parsed: %s [%s] %s\n", hostname, port, path);
  request(fd, hostname, port, path, hdrs);
  close(fd);
}

// ---

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
  char host[MAXLINE] = "", serv[MAXLINE] = "";
  while (1)
  {
    printf("Wating...\n");
    socklen_t addr_len = sizeof(client_addr);
    connfd = accept(listenfd, (SA *)&client_addr, &addr_len);
    getnameinfo((SA *)&client_addr, addr_len, host, MAXLINE, serv, MAXLINE, 0);
    printf("Connected (%s : %s)\n", host, serv);
    request_proxy(connfd);
    close(connfd);
  }

  return 0;
}
