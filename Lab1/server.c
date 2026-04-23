#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENT 100

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int randNum = rand() % 100 + 1;

    // ports 0-1024 are reserved
    int port = 1025 + rand() % (65535 - 1025);
    printf("Server will listen on port: %d\n", port);
    printf("Random number is: %i\n", randNum);

    int l_socket = socket(AF_INET, SOCK_STREAM, 0);

    // accept connections on any local interface
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(l_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        return 1;
    }

    // initialize all fds to -1 so poll() ignores unused slots
    struct pollfd *fds = malloc(sizeof *fds * MAX_CLIENT);
    for (int i = 0; i < MAX_CLIENT; ++i)
    {
        fds[i].events = POLLIN;
        fds[i].fd = -1;
    }

    // slot 0 reserved for listening socket, clients use slots 1+
    fds[0].fd = l_socket;

    int max_index = 0;
    char recv_buffer[1024];

    listen(l_socket, 5);
    while (1)
    {
        int numEvents = poll(fds, max_index + 1, -1);
        if (numEvents > 0)
        {
            // new client connecting
            if (fds[0].revents & POLLIN)
            {
                struct sockaddr_in clientaddr;
                socklen_t clientaddrlen = sizeof(clientaddr);

                // try to reuse a freed slot first
                int placed = 0;
                for (int i = 1; i <= max_index; ++i)
                {
                    if (fds[i].fd == -1)
                    {
                        fds[i].fd = accept(l_socket, (struct sockaddr *)&clientaddr, &clientaddrlen);
                        placed = 1;
                        break;
                    }
                }

                // no free slot, extend array
                if (!placed)
                {
                    if (max_index < MAX_CLIENT)
                    {
                        max_index++;
                        fds[max_index].fd = accept(l_socket, (struct sockaddr *)&clientaddr, &clientaddrlen);
                    }
                }
                numEvents--;
            }

            for (int i = 1; i <= max_index && numEvents > 0; ++i)
            {
                if (fds[i].revents & POLLIN)
                {
                    numEvents--;

                    // recv returns 0 if client closed, -1 on error
                    int s_len = recv(fds[i].fd, recv_buffer, sizeof(recv_buffer) - 1, 0);

                    if (s_len <= 0)
                    {
                        close(fds[i].fd);
                        fds[i].fd = -1;

                        if (i == max_index)
                        {
                            max_index--;
                        }

                        continue;
                    }

                    recv_buffer[s_len] = '\0';
                    int guess = atoi(recv_buffer);
                    printf("Someone guessed %i\n", guess);

                    char response[1024];
                    if (guess < 1 || guess > 100)
                    {
                        sprintf(response, "Number must be between 1 and 100\n");
                    }
                    else if (guess < randNum)
                    {
                        sprintf(response, "Too low\n");
                    }
                    else if (guess > randNum)
                    {
                        sprintf(response, "Too high\n");
                    }
                    else
                    {
                        // notify winner
                        sprintf(response, "You guessed correctly! Number was %d\n", randNum);
                        send(fds[i].fd, response, strlen(response), 0);
                        close(fds[i].fd);

                        // notify and close all other clients
                        sprintf(response, "\nSomeone else guessed it... Number was %d\n", randNum);
                        for (int j = 1; j <= max_index; ++j)
                        {
                            if (j == i || fds[j].fd == -1)
                            {
                                continue;
                            }
                            send(fds[j].fd, response, strlen(response), 0);
                            close(fds[j].fd);
                        }
                        printf("The correct number was guessed. Ending game\n");
                        return 0;
                    }

                    send(fds[i].fd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}