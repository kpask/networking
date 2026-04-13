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
    int l_socket;
    int client_count = 0;
    int randNum = rand() % 100 + 1;
    char buffer[1024];

    struct sockaddr_in servaddr;
    struct pollfd *fds = malloc(sizeof *fds * MAX_CLIENT);

    int port = 1025 + rand() % (65535 - 1025);
    printf("Server will listen on port: %d\n", port);

    // atidarom tcp socket
    l_socket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Random number is: %i", randNum);

    // bindam prie visu ip ir pasirinkto porto
    if (bind(l_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        return 1;
    }

    // inicializuojam fds, visi fd = -1 kad poll() ignoruotu
    for (int i = 0; i < MAX_CLIENT; ++i)
    {
        fds[i].events = POLLIN;
        fds[i].fd = -1;
    }

    // fds[0] rezervuotas listening socketui
    fds[0].fd = l_socket;
    fds[0].events = POLLIN;

    listen(l_socket, 5);

    while (1)
    {
        // tikrinam tik aktyvius fds: l_socket + prisijunge klientai
        int numEvents = poll(fds, client_count + 1, -1);
        if (numEvents > 0)
        {
            // naujas klientas jungiasi
            if (fds[0].revents & POLLIN)
            {
                if (client_count == MAX_CLIENT)
                {
                    continue;
                }

                struct sockaddr_in clientaddr;
                socklen_t clientaddrlen = sizeof(clientaddr);

                // priimam klienta, jo fd dedame i kita laisva vieta
                fds[client_count + 1].fd = accept(l_socket, (struct sockaddr *)&clientaddr, &clientaddrlen);
                client_count++;

                numEvents--;
                if (numEvents == 0)
                {
                    continue;
                }
            }

            // tikrinam klientu atsakymus
            for (int i = 1; i <= client_count && numEvents > 0; ++i)
            {
                if (fds[i].revents & POLLIN)
                {
                    numEvents--;

                    char response[1024];
                    int s_len = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    // klientas atsijunge
                    if (s_len <= 0)
                    {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        continue;
                    }

                    buffer[s_len] = '\0';
                    int guess = atoi(buffer);

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
                        // teisingas atspejimas - pranesam laimejejui
                        sprintf(response, "You guessed correctly! Number was %d", randNum);
                        send(fds[i].fd, response, strlen(response), 0);
                        close(fds[i].fd);

                        // pranesam likusiems klientams
                        sprintf(response, "Someone else guessed it... Number was %d\n", randNum);
                        for (int j = 1; j <= client_count; ++j)
                        {
                            if (j == i || fds[j].fd == -1)
                            {
                                continue;
                            }
                            send(fds[j].fd, response, strlen(response), 0);
                            close(fds[j].fd);
                        }
                        return 0;
                    }

                    send(fds[i].fd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}