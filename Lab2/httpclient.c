#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFLEN 1024

void parse_url(const char *url, char *host, char *port, char *path)
{
    // skip "http://"
    const char *start = strstr(url, "://");
    start = start ? start + 3 : url;

    // check for port — find colon before first slash
    const char *colon = strchr(start, ':');
    const char *slash = strchr(start, '/');

    if (colon && (!slash || colon < slash))
    {
        // host:port/path
        strncpy(host, start, colon - start);
        host[colon - start] = '\0';

        const char *port_end = slash ? slash : colon + strlen(colon);
        strncpy(port, colon + 1, port_end - (colon + 1));
        port[port_end - (colon + 1)] = '\0';
    }
    else
    {
        // no port, default 80
        if (slash)
        {
            strncpy(host, start, slash - start);
            host[slash - start] = '\0';
        }
        else
        {
            strcpy(host, start);
        }
        strcpy(port, "80");
    }

    // path after first slash
    if (slash)
    {
        strcpy(path, slash + 1);
    }

    else
    {
        path[0] = '\0';
    }

    // remove trailing slash
    int path_len = strlen(path);
    if (path_len > 0 && path[path_len - 1] == '/')
    {
        path[path_len - 1] = '\0';
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;

    if (argc != 2)
    {
        printf("Usage: %s <url>\n", argv[0]);
        printf("  e.g. %s http://example.com/image.jpg\n", argv[0]);
        printf("  e.g. %s localhost:8080/file.png\n", argv[0]);
        return 1;
    }

    if (strncmp(argv[1], "https://", 8) == 0)
    {
        fprintf(stderr, "HTTPS not supported\n");
        return 1;
    }

    char host[256] = {0};
    char port[16] = {0};
    char path[1024] = {0};
    parse_url(argv[1], host, port, path);

    char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    if (strlen(filename) == 0)
    {
        filename = "index.html";
    }

    // resolve hostname to IP via DNS
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res))
    {
        fprintf(stderr, "getaddrinfo error");
        return 1;
    }

    // open TCP socket using resolved address info
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen))
    {
        perror("connect");
        freeaddrinfo(res);
        close(sock);
        return 1;
    }
    freeaddrinfo(res);

    // build and send HTTP GET request
    char request[BUFFLEN];
    snprintf(request, BUFFLEN, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    printf("Sending request:\n%s\n", request);
    send(sock, request, strlen(request), 0);

    // receive full response into dynamically growing buffer
    char *response = malloc(sizeof(char) * 1024);
    int current_capacity = 1024;
    int received = 0;
    response[0] = '\0';
    char buffer[4096];
    int n;

    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        // grow buffer if needed
        while (n + received > current_capacity)
        {
            current_capacity *= 2;
            char *tmp = realloc(response, current_capacity);
            if (!tmp)
            {
                perror("realloc");
                free(response);
                close(sock);
                return 1;
            }
            response = tmp;
        }
        memcpy(response + received, buffer, n);
        received += n;
    }

    // realoc + 1 byte incase its full
    char *tmp = realloc(response, received + 1);
    if (!tmp)
    {
        printf("Error allocating memory.\n");
    }
    response = tmp;
    response[received] = '\0';

    // after receiving we check if status code is < 400 - successful
    if (strncmp(response, "HTTP/", 5) == 0)
    {
        int status;
        sscanf(response, "HTTP/%*s %d", &status);
        if (status >= 400)
        {
            printf("Server returned %d\n", status);
            free(response);
            close(sock);
            return 1;
        }
    }

    // find end of headers — body starts after \r\n\r\n
    char *body = strstr(response, "\r\n\r\n");
    if (!body)
    {
        printf("Invalid HTTP response\n");
        free(response);
        close(sock);
        return 1;
    }
    body += 4;

    // write body to file in binary mode to preserve all bytes
    FILE *fptr = fopen(filename, "wb");
    if (!fptr)
    {
        perror("fopen");
        free(response);
        close(sock);
        return 1;
    }

    int body_len = received - (body - response);
    fwrite(body, 1, body_len, fptr);
    printf("Saved %d bytes to %s\n", body_len, filename);

    free(response);
    close(sock);
    fclose(fptr);
    return 0;
}