#pragma once
#include <pthread.h>
#include <time.h>

#define MAX_ROUTERS 10
#define MAX_NEIGHBORS 10
#define INFINITY 9999

struct Neighbor
{
    int id;
    int port;
    time_t last_seen;
};

struct Router
{
    int id;
    int port;
    int isActive;
    int sock;
    pthread_t thread;
    int dist[MAX_ROUTERS];
    int next_hop[MAX_ROUTERS];
    struct Neighbor neighbors[MAX_NEIGHBORS];
};

struct RouteUpdate
{
    int from_id;
    int dist[MAX_ROUTERS];
};

/* Global variables */
extern int router_count;
extern struct Router routers[MAX_ROUTERS];

/* Function signatures */
void init_network();
int is_port_available(int port);
int create_router(int port);
void remove_router(int id);
void connect_routers(int a, int b);
void disconnect_routers(int a, int b);
void print_table(int id);
void update_distances(int id, int from_id, int *received_dist);
int is_neighbor(int id, int other);
