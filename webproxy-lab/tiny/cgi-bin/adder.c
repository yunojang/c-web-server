/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  const char *head_env = getenv("IS_HEAD");
  int is_head = strcmp(head_env, "1") == 0;

  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  if ((buf = getenv("QUERY_STRING")) == NULL)
  {
    return -1;
  }

  sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);

  /* Extract the two arguments */
  p = strchr(buf, '&');
  if (p < 0)
  {
    return -1;
  }
  *p = '\0';
  strcpy(arg1, buf);
  strcpy(arg2, p + 1);
  // n1 = atoi(arg1);
  // n2 = atoi(arg2);
  n1 = atoi(strchr(arg1, '=') + 1);
  n2 = atoi(strchr(arg2, '=') + 1);

  /* Make the response body */
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");
  sprintf(content + strlen(content), "<form method=\"GET\">\r\n");
  sprintf(content + strlen(content), "<input type=\"text\" name=\"a\">\r\n");
  sprintf(content + strlen(content), "<input type=\"text\" name=\"b\">\r\n");
  sprintf(content + strlen(content), "<input type=\"submit\">\r\n");
  sprintf(content + strlen(content), "</form>\r\n");

  /* Generate the HTTP response */
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");

  if (!is_head)
  {
    printf("%s", content);
  }
  fflush(stdout);

  exit(0);
}
/* $end adder */
