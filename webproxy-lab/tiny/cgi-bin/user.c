#include "csapp.h"

typedef struct _user
{
    char *name;
    int age;
} User;

User user[2] = {{"yunojang", 26}, {"hi", 23}};

int main()
{
    char *qs_buf;
    char key[MAXBUF], val[MAXBUF], content[MAXBUF];
    if ((qs_buf = getenv("QUERY_STRING")) == NULL)
    {
        // fprintf(stderr, "no querystring \n");
        return 2;
    }

    char *sep_ptr;
    if ((sep_ptr = strchr(qs_buf, '=')) < 0)
    {
        // fprintf(stderr, "no seperator\n");
        return 3;
    }
    *sep_ptr = '\0';
    strcpy(key, qs_buf);
    strcpy(val, sep_ptr + 1);

    if (strcmp(key, "userId") || strchr(val, '='))
    {
        return 4;
    }
    size_t max_n = sizeof user / sizeof user[0];
    if (val < 0 || (size_t)atoi(val) >= max_n)
    {
        return 5;
    }

    User u = user[atoi(val)];
    sprintf(content + strlen(content), "Welcome to User \r\n");
    sprintf(content + strlen(content), "user %s: \r\n", val);
    sprintf(content + strlen(content), "name: %s \r\n", u.name);
    sprintf(content + strlen(content), "age: %d \r\n", u.age);

    printf("Content-type: text/html\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}