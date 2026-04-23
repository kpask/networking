#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "router.h"

int router_count = 0;
struct Router routers[MAX_ROUTERS];

/*
 * Initializes the entire router array.
 * Sets all distances to INFINITY (unknown) and next_hop to -1.
 */
void init_network()
{
    memset(routers, 0, sizeof(routers));
    for (int i = 0; i < MAX_ROUTERS; ++i)
    {
        for (int j = 0; j < MAX_ROUTERS; ++j)
        {
            routers[i].dist[j] = INFINITY;
            routers[i].next_hop[j] = -1;
        }

        // set all neighbor ids to -1, since memset made it 0 - a valid router id
        for (int j = 0; j < MAX_NEIGHBORS; ++j)
        {
            routers[i].neighbors[j].id = -1;
        }
    }
}

/*
 * Checks if a port is available on the system.
 * Creates a temporary socket and attempts to bind it.
 * Returns 1 if available, 0 if occupied.
 */
int is_port_available(int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return 0;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    close(sock);
    return result == 0;
}

/*
 * Creates a new router with the specified port.
 * Returns the router ID or -1 if failed.
 * Router is not yet active - start_router() must be called.
 */
int create_router(int port)
{
    int id = -1;
    for (int i = 0; i < MAX_ROUTERS; ++i)
    {
        if (routers[i].port == 0)
        {
            id = i;
            break;
        }
    }

    if (id == -1)
    {
        printf("Max routers reached\n");
        return -1;
    }

    if (port <= 1023)
    {
        printf("When adding a router use a port bigger than 1023\n");
        return -1;
    }

    if (!is_port_available(port))
    {
        printf("Port %d is already in use by another process\n", port);
        return -1;
    }

    for (int i = 0; i < MAX_ROUTERS; i++)
    {
        if (routers[i].port == port)
        {
            printf("Port %d is already assigned to router %d\n", port, i);
            return -1;
        }
    }

    routers[id].id = id;
    routers[id].port = port;
    routers[id].isActive = 0;
    routers[id].dist[id] = 0;
    routers[id].next_hop[id] = id;
    router_count++;

    return id;
}

/*
 * Disconnects a direct link between two routers.
 * Removes neighbor entries in both directions
 * and sets the distance to INFINITY.
 */
void disconnect_routers(int a, int b)
{
    /* Remove b from a's neighbors */
    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        if (routers[a].neighbors[i].id == b)
        {
            routers[a].neighbors[i].id = -1;
            routers[a].neighbors[i].port = 0;
            routers[a].dist[b] = INFINITY;
            routers[a].next_hop[b] = -1;
            break;
        }
    }
    /* Remove a from b's neighbors */
    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        if (routers[b].neighbors[i].id == a)
        {
            routers[b].neighbors[i].id = -1;
            routers[b].neighbors[i].port = 0;
            routers[b].dist[a] = INFINITY;
            routers[b].next_hop[a] = -1;
            break;
        }
    }

    // any hop from a through b is terminated
    for (int i = 0; i < MAX_ROUTERS; ++i)
    {
        if (routers[a].next_hop[i] == b)
        {
            routers[a].next_hop[i] = -1;
            routers[a].dist[i] = INFINITY;
        }
    }
    // same for b
    for (int i = 0; i < MAX_ROUTERS; ++i)
    {
        if (routers[b].next_hop[i] == a)
        {
            routers[b].next_hop[i] = -1;
            routers[b].dist[i] = INFINITY;
        }
    }
}

/*
 * Removes a router from the network: disconnects all neighbors
 * and turns off the isActive flag.
 */
void remove_router(int id)
{
    if (id < 0 || id >= MAX_ROUTERS)
    {
        return;
    }

    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        if (routers[id].neighbors[i].port != 0)
        {
            disconnect_routers(id, routers[id].neighbors[i].id);
        }
    }

    routers[id].isActive = 0;
    routers[id].port = 0;
    router_count--;
}

/*
 * Connects two routers with a direct link.
 * Creates a neighbor entry for each router.
 * Initial distance = 1, next_hop = direct neighbor.
 */
void connect_routers(int a, int b)
{
    if (a < 0 || a >= MAX_ROUTERS || b < 0 || b >= MAX_ROUTERS || a == b || !routers[a].isActive || !routers[b].isActive)
    {
        return;
    }

    int slot_a = -1, slot_b = -1;

    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        /* Check if already connected */
        if (routers[a].neighbors[i].id == b && routers[a].neighbors[i].port != 0)
        {
            return;
        }

        /* Find available slots for both */
        if (slot_a == -1 && routers[a].neighbors[i].port == 0)
        {
            slot_a = i;
        }
        if (slot_b == -1 && routers[b].neighbors[i].port == 0)
        {
            slot_b = i;
        }
    }

    if (slot_a == -1 || slot_b == -1)
    {
        printf("No space for new neighbor\n");
        return;
    }

    routers[a].neighbors[slot_a] = (struct Neighbor){b, routers[b].port, time(NULL)};
    routers[b].neighbors[slot_b] = (struct Neighbor){a, routers[a].port, time(NULL)};

    routers[a].dist[b] = 1;
    routers[a].next_hop[b] = b;
    routers[b].dist[a] = 1;
    routers[b].next_hop[a] = a;
}

/*
 * Prints the routing table for the specified router.
 * Shows distance and next_hop for every active router.
 */
void print_table(int id)
{
    printf("Router %d routing table:\n", id);
    for (int i = 0; i < MAX_ROUTERS; i++)
    {
        if (!routers[i].isActive)
        {
            continue;
        }
        if (routers[id].dist[i] == INFINITY)
        {
            printf("  -> %d : unreachable\n", i);
        }
        else
        {
            printf("  -> %d : dist=%d next_hop=%d\n",
                   i, routers[id].dist[i], routers[id].next_hop[i]);
        }
    }
}

/*
 * Bellman-Ford update upon receiving a neighbor's distance vector.
 * If a better path is found - update our table.
 * If the old path was through this neighbor - update or remove the route.
 */
void update_distances(int id, int from_id, int *received_dist)
{
    if (!is_neighbor(id, from_id))
    {
        return;
    }

    int changed = 0;
    for (int k = 0; k < MAX_ROUTERS; k++)
    {
        /* Skip the neighbor's distance to itself and our distance to ourselves */
        if (k == id || k == from_id)
        {
            continue;
        }

        int new_dist = received_dist[k] == INFINITY ? INFINITY : routers[id].dist[from_id] + received_dist[k];

        /*
         * If the old path to k was via this neighbor,
         * update to the new distance or close the path.
         */
        if (routers[id].next_hop[k] == from_id)
        {
            if (routers[id].dist[k] != new_dist)
            {
                routers[id].dist[k] = new_dist;
                routers[id].next_hop[k] = (new_dist == INFINITY) ? -1 : from_id;
                changed = 1;
            }
        }
        /*
         * If the new path is shorter, change the route
         * to go through this neighbor.
         */
        else if (new_dist < routers[id].dist[k])
        {
            routers[id].dist[k] = new_dist;
            routers[id].next_hop[k] = from_id;
            changed = 1;
        }
    }

    if (changed)
    {
        print_table(id);
    }
}

/*
 * Checks if the specified router is a direct neighbor.
 */
int is_neighbor(int id, int other)
{
    for (int i = 0; i < MAX_NEIGHBORS; i++)
    {
        if (routers[id].neighbors[i].id == other &&
            routers[id].neighbors[i].port != 0)
        {
            return 1;
        }
    }
    return 0;
}