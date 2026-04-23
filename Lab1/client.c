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

    // Create TCP socket
    int port = atoi(argv[2]);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    // Setup server address
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_aton(argv[1], &server.sin_addr) == 0)
    {
        fprintf(stderr, "Invalid IP\n");
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        return 1;
    }

    // Monitor stdin and socket
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock;
    pfds[1].events = POLLIN;

    printf("Guess the number (1-100): ");
    fflush(stdout);
    char buffer[BUFFLEN];
    while (1)
    {
        // Wait for input (user or server)
        if (poll(pfds, 2, -1) < 0)
        {
            perror("poll");
            break;
        }

        // Data from server
        if (pfds[1].revents & POLLIN)
        {
            int n = recv(sock, buffer, BUFFLEN - 1, 0);
            if (n <= 0)
            {
                printf("Server disconnected\n");
                break;
            }

            buffer[n] = '\0';
            printf("%s", buffer);

            // Exit if game ended
            if (strstr(buffer, "You guessed") || strstr(buffer, "Someone else"))
            {
                break;
            }

            printf("Guess the number (1-100): ");
            fflush(stdout);
        }

        // User input
        if (pfds[0].revents & POLLIN)
        {
            if (!fgets(buffer, BUFFLEN, stdin))
            {
                break;
            }

            if (send(sock, buffer, strlen(buffer), 0) < 0)
            {
                perror("send");
                break;
            }
        }
    }

    // Close connection
    close(sock);
    return 0;
}