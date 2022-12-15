#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_HASH_TABLE_SIZE (1 << 16)

/* You won't lose style points for including this long line in your code */
static const char* user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* request_line_format = "GET %s HTTP/1.0\r\n";
static const char* host_header_format = "Host: %s\r\n";
static const char* end_of_header = "\r\n";

void process(int fd);
void parse_uri(char* uri, char* hostname, int* port, char* path);
void set_http_request_header(char* http_header, char* hostname, int* port, char* path, rio_t* rio);
int connect_to_server(char* hostname, int port);

void* thread_main(void* targs);
unsigned int get_hash_key(char* string);
void set_table_entry(unsigned int hash_key);


typedef struct cached_data
{
    char is_used;
    char data[MAX_OBJECT_SIZE];
}cached_data_t;

cached_data_t cache_table[MAX_HASH_TABLE_SIZE] = { 0 };

int main(int argc, char** argv)
{
    printf("%s", user_agent_hdr);
    char hostname[MAXLINE], port[MAXLINE];
    int fd_listen;
    // Client to Proxy
    int fd_client;

    socklen_t client_len;
    struct sockaddr_storage client_addr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    fd_listen = Open_listenfd(argv[1]);
    while (1)
    {
        client_len = sizeof(client_addr);
        fd_client = Accept(fd_listen, (SA*)&client_addr, &client_len);
        Getnameinfo((SA*)&client_addr, client_len, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n", hostname, port);
        // Do forward and reverse
        pthread_t thread;
        pthread_create(&thread, NULL, thread_main, &fd_client);
    }

    return 0;
}

void process(int fd)
{
    // Proxy to Server
    int fd_server;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char http_header_to_server[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    int port = 0;

    rio_t rio_client;
    rio_t rio_server;

    Rio_readinitb(&rio_client, fd);
    Rio_readlineb(&rio_client, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET"))
    {
        printf("[%s] This method isn't implemented on Proxy server.\n", method);
        return;
    }
    //if hit cache
    unsigned int hash_key = get_hash_key(uri);
    if (cache_table[hash_key].is_used)
    {
        char* cached_data_buffer = cache_table[hash_key].data;
        Rio_writen(fd, cached_data_buffer, strlen(cached_data_buffer));
        return;
    }

    parse_uri(uri, hostname, &port, path);
    set_http_request_header(http_header_to_server, hostname, &port, path, &rio_client);

    fd_server = connect_to_server(hostname, port);
    if (fd_server < 0)
    {
        printf("Connection failed");
        return;
    }

    Rio_readinitb(&rio_server, fd_server);
    Rio_writen(fd_server, http_header_to_server, strlen(http_header_to_server));

    set_table_entry(hash_key);
    char* cache_buf = cache_table[hash_key].data;

    size_t len;
    while ((len = Rio_readlineb(&rio_server, buf, MAXLINE)))
    {
        printf("Proxy received %ld Bytes and send\n", len);
        Rio_writen(fd, buf, len);
        memcpy(cache_buf, buf, len);
        cache_buf += len;
        //...
    }
    Close(fd_server);
}

void parse_uri(char* uri, char* hostname, int* port, char* path)
{
    // ex. https://localhost:8884 <=> 192.168.0.1:8884 
    char* port_pos = '\0';
    char* path_pos = '\0';
    char* hostname_pos = strstr(uri, "//");
    // https://localhost~ vs localhost~
    hostname_pos = hostname_pos ? hostname_pos + 2 : uri;
    port_pos = strstr(hostname_pos, ":");
    // localhost:8884~
    if (port_pos)
    {
        *port_pos = '\0';
        sscanf(hostname_pos, "%s", hostname);
        sscanf(port_pos + 1, "%d%s", port, path);
    }
    //localhost~
    else
    {
        path_pos = strstr(hostname_pos, "/");
        if (path_pos)
        {
            *path_pos = '\0';
            sscanf(hostname_pos, "%s", hostname);
            sscanf(path_pos, "%s", path);
        }
        else
        {
            sscanf(hostname_pos, "%s", hostname);
        }
    }
}

void set_http_request_header(char* http_header, char* hostname, int* port, char* path, rio_t* rio_client)
{
    char buf[MAXLINE];
    char request_header[MAXLINE];
    char general_header[MAXLINE];
    char host_header[MAXLINE];

    sprintf(request_header, request_line_format, path);
    while (Rio_readlineb(rio_client, buf, MAXLINE))
    {
        if (!strcmp(buf, end_of_header))
        {
            break;
        }
        strcat(general_header, buf);
    }

    if (strlen(host_header) == 0)
    {
        sprintf(host_header, host_header_format, hostname);
    }

    sprintf(http_header, "%s%s%s%s",
        request_header,
        user_agent_hdr,
        general_header,
        end_of_header
    );
}

int connect_to_server(char* hostname, int port)
{
    char port_string[8];
    sprintf(port_string, "%d", port);
    return Open_clientfd(hostname, port_string);
}

void* thread_main(void* targs)
{
    pthread_detach(pthread_self());
    process(*(int*)targs);

    Close(*(int*)targs);

    return NULL;
}

unsigned int get_hash_key(char* string)
{
    unsigned long long hash = 5381;
    char* ptr = string;
    while (*ptr)
    {
        hash = ((hash << 5) + hash) + *ptr;
        ptr++;
    }
    return (unsigned int)(hash % MAX_HASH_TABLE_SIZE);
}

void set_table_entry(unsigned int hash_key)
{
    cache_table[hash_key].is_used = 1;
    memset(cache_table[hash_key].data, 0, MAX_OBJECT_SIZE);
}