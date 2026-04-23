#include "router.h"

void start_router(int id);          // starts router thread
void stop_router(int id);           // stops router thread
void broadcast_update(int id);      // broadcasts dist[] to neighbors
void *router_thread(void *arg);     // thread function
void send_packet(int src, int dst); // simulates packet transmission
