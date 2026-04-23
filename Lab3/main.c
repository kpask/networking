#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "network.h"
#include "router.h"

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  init_network();

  char buffer[1024];

  while (1)
  {
    int option;

    printf("\n=== Routing Simulator ===\n");
    printf("1 - create router\n");
    printf("2 - connect routers\n");
    printf("3 - print router distance table\n");
    printf("4 - disconnect routers\n");
    printf("5 - remove router\n");
    printf("6 - send packet\n");
    printf("7 - list active routers\n");
    printf("Enter option: ");
    fflush(stdout);

    fgets(buffer, sizeof(buffer), stdin);
    option = atoi(buffer);

    switch (option)
    {
    case 1:
    {
      printf("Enter router port: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int port = atoi(buffer);

      if (port <= 1023 || port > 65535)
      {
        printf("Invalid port entered. Use (1024 - 65535)\n");
        continue;
      }

      int id = create_router(port);
      if (id >= 0)
      {
        start_router(id);
      }
      break;
    }
    case 2:
    {
      printf("Enter router A id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int a = atoi(buffer);
      printf("Enter router B id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int b = atoi(buffer);
      connect_routers(a, b);
      break;
    }
    case 3:
    {
      printf("Enter router id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int id = atoi(buffer);
      print_table(id);
      break;
    }
    case 4:
    {
      printf("Enter router A id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int a = atoi(buffer);
      printf("Enter router B id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int b = atoi(buffer);
      disconnect_routers(a, b);
      break;
    }
    case 5:
    {
      printf("Enter router id to remove: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int id = atoi(buffer);
      stop_router(id);
      remove_router(id);
      break;
    }
    case 6:
    {
      printf("Enter source router id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int src = atoi(buffer);
      printf("Enter destination router id: ");
      fflush(stdout);
      fgets(buffer, sizeof(buffer), stdin);
      int dst = atoi(buffer);
      send_packet(src, dst);
      break;
    }
    case 7:
    {
      printf("Active routers:\n");
      for (int i = 0; i < MAX_ROUTERS; i++)
      {
        if (routers[i].isActive)
        {
          printf("  Router %d on port %d\n", i, routers[i].port);
        }
      }
      break;
    }
    default:
      printf("Invalid option\n");
      break;
    }
  }
}
