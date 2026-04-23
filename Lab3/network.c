
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "router.h"
#include <netinet/in.h>
#include <poll.h>

void broadcast_update(int id)
{
    if (id < 0 || id >= MAX_ROUTERS || !routers[id].isActive)
    {
        return;
    }

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    inet_aton("127.0.0.1", &dest.sin_addr);

    // Iterate through all neighbors to send them our table
    for (int i = 0; i < MAX_NEIGHBORS; ++i)
    {
        int neighbor_id = routers[id].neighbors[i].id;
        int neighbor_port = routers[id].neighbors[i].port;

        if (neighbor_port == 0 || neighbor_id == -1)
        {
            continue;
        }

        struct RouteUpdate update;
        update.from_id = id;

        /* * SPLIT HORIZON IMPLEMENTATION:
         * If we are sending an update to Neighbor B, and our best path
         * to a destination X actually goes THROUGH Neighbor B, we
         * tell Neighbor B that X is INFINITY.
         * This prevents routing loops.
         */
        for (int k = 0; k < MAX_ROUTERS; k++)
        {
            if (routers[id].next_hop[k] == neighbor_id && k != neighbor_id)
            {
                update.dist[k] = INFINITY;
            }
            else
            {
                update.dist[k] = routers[id].dist[k];
            }
        }

        dest.sin_port = htons(neighbor_port);
        sendto(routers[id].sock, &update, sizeof(update), 0,
               (struct sockaddr *)&dest, sizeof(dest));
    }
}

void *router_thread(void *arg)
{
    int id = *(int *)arg;
    struct pollfd pfd;
    pfd.fd = routers[id].sock;
    pfd.events = POLLIN;

    time_t last_periodic_check = 0;

    while (routers[id].isActive)
    {
        int n = poll(&pfd, 1, 500);

        // 1. PROCESS ALL INCOMING DATA
        if (n > 0 && (pfd.revents & POLLIN))
        {
            struct RouteUpdate update;
            // Drain the socket buffer entirely
            while (recvfrom(routers[id].sock, &update, sizeof(update), MSG_DONTWAIT, NULL, NULL) > 0)
            {
                for (int i = 0; i < MAX_NEIGHBORS; i++)
                {
                    if (routers[id].neighbors[i].id == update.from_id)
                    {
                        routers[id].neighbors[i].last_seen = time(NULL);
                        break;
                    }
                }
                update_distances(id, update.from_id, update.dist);
            }
        }

        // 2. PERIODIC TASKS (Every 2 seconds)
        time_t now = time(NULL);
        if (difftime(now, last_periodic_check) >= 2)
        {
            // Send our presence to neighbors
            broadcast_update(id);

            // Check for dead neighbors
            for (int i = 0; i < MAX_NEIGHBORS; i++)
            {
                if (routers[id].neighbors[i].port != 0 &&
                    difftime(now, routers[id].neighbors[i].last_seen) > 10) // Relaxed to 10s
                {
                    printf("\nRouter %d: neighbor %d timed out (last seen %ld sec ago)!\n",
                           id, routers[id].neighbors[i].id, (long)difftime(now, routers[id].neighbors[i].last_seen));
                    disconnect_routers(id, routers[id].neighbors[i].id);
                }
            }
            last_periodic_check = now;
        }
    }

    close(routers[id].sock);
    return NULL;
}

void start_router(int id)
{
    if (id < 0 || id >= MAX_ROUTERS || routers[id].port <= 1023)
    {
        printf("Router %i doesn't exist.\n", id);
        return;
    }

    if (routers[id].isActive)
    {
        printf("Router %i is already active", id);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(routers[id].port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return;
    }

    routers[id].sock = sock;
    routers[id].isActive = 1;

    /* Initialize last_seen to the current time */
    time_t now = time(NULL);
    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        if (routers[id].neighbors[i].port != 0)
            routers[id].neighbors[i].last_seen = now;
    }

    pthread_create(&routers[id].thread, NULL, router_thread, &routers[id].id);
}

/*
 * Stops the router thread and clears isActive.
 * The thread itself will close the socket when isActive == 0.
 */
void stop_router(int id)
{
    if (id < 0 || id >= MAX_ROUTERS || !routers[id].isActive)
    {
        printf("Router %i is not active\n", id);
        return;
    }

    routers[id].isActive = 0;
    pthread_join(routers[id].thread, NULL);
    printf("Router %d stopped\n", id);
}

/*
 * Simulation of packet sending through the network.
 * Prints the path based on the next_hop table.
 */
void send_packet(int src, int dst)
{
    if (src < 0 || src >= MAX_ROUTERS || !routers[src].isActive)
    {
        printf("Source router %d is not active\n", src);
        return;
    }
    if (dst < 0 || dst >= MAX_ROUTERS || !routers[dst].isActive)
    {
        printf("Destination router %d is not active\n", dst);
        return;
    }

    printf("Sending packet from %d to %d: ", src, dst);
    int current = src;
    int hops = 0;

    while (current != dst)
    {
        if (routers[current].dist[dst] == INFINITY || routers[current].next_hop[dst] == -1)
        {
            printf("UNREACHABLE at router %d\n", current);
            return;
        }

        printf("%d -> ", current);
        current = routers[current].next_hop[dst];
        hops++;

        if (hops > MAX_ROUTERS)
        {
            printf("LOOP DETECTED\n");
            return;
        }
    }

    printf("%d (arrived in %d hops)\n", dst, hops);
}
