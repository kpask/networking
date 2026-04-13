#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#define BUFFLEN 1024

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    // set connection for ipv4, network order port
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // set to binary the connection address
    if (inet_aton(argv[1], &server.sin_addr) == 0)
    {
        fprintf(stderr, "Invalid IP address\n");
        return 1;
    }

    // open connection fd to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        return 1;
    }

    // set up polls, one for user input other for server
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO; // keyboard
    pfds[0].events = POLLIN;
    pfds[1].fd = sock; // server
    pfds[1].events = POLLIN;

    // print and fflush to print bufered stdout
    printf("Guess the number (1-100): ");
    fflush(stdout);
    while (1)
    {
        char buffer[BUFFLEN];

        // poll sockets, check if any occured
        poll(pfds, 2, -1);

        // server has sent something
        if (pfds[1].revents & POLLIN)
        {
            // check return of recv - sent buffer size
            int n = recv(sock, buffer, BUFFLEN - 1, 0);
            if (n <= 0)
            {
                printf("Server disconnected\n");
                break;
            }

            buffer[n] = '\0';
            printf("%s\n", buffer);

            // compare check response, if the game has ended, finish the cycle and close connection
            if (strncmp(buffer, "You guessed", 11) == 0 ||
                strncmp(buffer, "Someone else", 12) == 0)
            {
                break;
            }

            // number is not guessed send prompt to enter again
            printf("Guess the number (1-100): ");
            fflush(stdout);
        }
        // user entered something
        if (pfds[0].revents & POLLIN)
        {
            // load from system buffer to our buffer
            fgets(buffer, BUFFLEN, stdin);
            // send the buffer to the server
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}