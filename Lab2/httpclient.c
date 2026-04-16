#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netdb.h>

#define BUFFLEN 1024

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;

    if (argc != 3)
    {
        printf("Usage: %s <domain.com> <file>\n", argv[0]);
        return 1;
    }

    // resolve hostname to IP via DNS
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(argv[1], "80", &hints, &res))
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
    snprintf(request, BUFFLEN, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", argv[2], argv[1]);
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
    FILE *fptr = fopen(argv[2], "wb");
    if (!fptr)
    {
        perror("fopen");
        free(response);
        close(sock);
        return 1;
    }

    int body_len = received - (body - response);
    fwrite(body, 1, body_len, fptr);
    printf("Saved %d bytes to %s\n", body_len, argv[2]);

    free(response);
    close(sock);
    fclose(fptr);
    return 0;
}