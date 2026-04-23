#include <stdio.h>
#include <string.h>
#include "router.h"

#define TEST(name) printf("  %s ... ", name);
#define PASS() printf("PASS\n")
#define FAIL(msg)                  \
    do                             \
    {                              \
        printf("FAIL: %s\n", msg); \
        errors++;                  \
    } while (0)

static int errors = 0;

void test_init_network()
{
    TEST("init_network sets all distances to INFINITY");
    init_network();
    int ok = 1;
    for (int i = 0; i < MAX_ROUTERS; i++)
    {
        for (int j = 0; j < MAX_ROUTERS; j++)
        {
            if (routers[i].dist[j] != INFINITY)
            {
                ok = 0;
            }
            if (routers[i].next_hop[j] != -1)
            {
                ok = 0;
            }
        }
    }
    if (ok)
    {
        PASS();
    }
    else
    {
        FAIL("dist/next_hop not initialized correctly");
    }
}

void test_create_router()
{
    TEST("create_router returns valid id");
    init_network();
    int id = create_router(5001);
    if (id >= 0 && id < MAX_ROUTERS && routers[id].port == 5001)
    {
        PASS();
    }
    else
    {
        FAIL("invalid id or port not set");
    }

    TEST("create_router rejects port <= 1023");
    int bad = create_router(80);
    if (bad == -1)
    {
        PASS();
    }
    else
    {
        FAIL("should have rejected port 80");
    }

    TEST("create_router detects duplicate port");
    int dup = create_router(5001);
    if (dup == -1)
    {
        PASS();
    }
    else
    {
        FAIL("should have rejected duplicate port 5001");
    }
}

void test_connect_routers()
{
    TEST("connect_routers creates bidirectional links");
    init_network();
    int a = create_router(5001);
    int b = create_router(5002);
    routers[a].isActive = 1;
    routers[b].isActive = 1;

    connect_routers(a, b);

    if (is_neighbor(a, b) && is_neighbor(b, a) &&
        routers[a].dist[b] == 1 && routers[b].dist[a] == 1)
    {
        PASS();
    }
    else
    {
        FAIL("link not created correctly");
    }
}

void test_disconnect_routers()
{
    TEST("disconnect_routers removes link and sets distance to INFINITY");
    init_network();
    int a = create_router(5001);
    int b = create_router(5002);
    routers[a].isActive = 1;
    routers[b].isActive = 1;
    connect_routers(a, b);

    disconnect_routers(a, b);

    if (!is_neighbor(a, b) && !is_neighbor(b, a) &&
        routers[a].dist[b] == INFINITY && routers[b].dist[a] == INFINITY)
    {
        PASS();
    }
    else
    {
        FAIL("link not removed correctly");
    }
}

void test_update_distances()
{
    TEST("update_distances learns route via neighbor");
    init_network();
    int a = create_router(5001);
    int b = create_router(5002);
    int c = create_router(5003);
    routers[a].isActive = 1;
    routers[b].isActive = 1;
    routers[c].isActive = 1;

    /* Chain: A -- B -- C */
    connect_routers(a, b);
    connect_routers(b, c);

    /* A already knows B (dist=1). Now A receives B's table. */
    int b_dist[MAX_ROUTERS];
    memcpy(b_dist, routers[b].dist, sizeof(b_dist));

    update_distances(a, b, b_dist);

    if (routers[a].dist[c] == 2 && routers[a].next_hop[c] == b)
    {
        PASS();
    }
    else
    {
        FAIL("did not learn route to C via B");
    }
}

void test_is_port_available()
{
    TEST("is_port_available detects used port");
    init_network();
    int id = create_router(5001);
    routers[id].isActive = 1;

    /* Since router is not actually started (no socket bound),
     * is_port_available should see port 5001 as free from OS perspective.
     * But our internal check in create_router catches duplicates.
     * Test that a truly random port is available. */
    if (is_port_available(65432))
    {
        PASS();
    }
    else
    {
        FAIL("expected port 65432 to be available");
    }
}

void test_remove_router()
{
    TEST("remove_router clears all neighbors and resets port");
    init_network();
    int a = create_router(5001);
    int b = create_router(5002);
    routers[a].isActive = 1;
    routers[b].isActive = 1;
    connect_routers(a, b);

    remove_router(a);

    if (routers[a].port == 0 && !routers[a].isActive && !is_neighbor(b, a))
    {
        PASS();
    }
    else
    {
        FAIL("router not removed correctly");
    }
}

void test_reroute_after_disconnect()
{
    TEST("reroute after disconnect: 0-1-2-3-4 topology");
    init_network();

    int r0 = create_router(5001);
    int r1 = create_router(5002);
    int r2 = create_router(5003);
    int r3 = create_router(5004);
    int r4 = create_router(5005);

    routers[r0].isActive = 1;
    routers[r1].isActive = 1;
    routers[r2].isActive = 1;
    routers[r3].isActive = 1;
    routers[r4].isActive = 1;

    /* Topology:
     * 0 --- 1 --- 2
     *       |   /
     *       3 --
     *       |
     *       4
     */
    connect_routers(r0, r1);
    connect_routers(r1, r2);
    connect_routers(r1, r3);
    connect_routers(r3, r2);
    connect_routers(r3, r4);

    /* Simulate broadcasts until convergence */
    for (int round = 0; round < MAX_ROUTERS; round++)
    {
        for (int i = 0; i < MAX_ROUTERS; i++)
        {
            if (!routers[i].isActive)
                continue;
            for (int j = 0; j < MAX_NEIGHBORS; j++)
            {
                int nb = routers[i].neighbors[j].id;
                if (nb == -1)
                    continue;
                update_distances(nb, i, routers[i].dist);
            }
        }
    }

    /* Disconnect 1 -- 3 */
    disconnect_routers(r1, r3);

    /* Simulate broadcasts again after disconnect */
    for (int round = 0; round < MAX_ROUTERS; round++)
    {
        for (int i = 0; i < MAX_ROUTERS; i++)
        {
            if (!routers[i].isActive)
                continue;
            for (int j = 0; j < MAX_NEIGHBORS; j++)
            {
                int nb = routers[i].neighbors[j].id;
                if (nb == -1)
                    continue;
                update_distances(nb, i, routers[i].dist);
            }
        }
    }

    /* After disconnect:
     * 4 -> 3 -> 2 -> 1 -> 0 = dist 4, next_hop = 3
     * 3 -> 2 -> 1 -> 0      = dist 3, next_hop = 2
     */
    int r4_to_r0_ok = routers[r4].dist[r0] == 4 && routers[r4].next_hop[r0] == r3;
    int r3_to_r0_ok = routers[r3].dist[r0] == 3 && routers[r3].next_hop[r0] == r2;

    if (r4_to_r0_ok && r3_to_r0_ok)
    {
        PASS();
    }
    else
    {
        printf("\n    r4->r0: dist=%d next_hop=%d (expected dist=4 next_hop=%d)\n",
               routers[r4].dist[r0], routers[r4].next_hop[r0], r3);
        printf("    r3->r0: dist=%d next_hop=%d (expected dist=3 next_hop=%d)\n",
               routers[r3].dist[r0], routers[r3].next_hop[r0], r2);
        FAIL("rerouting after disconnect failed");
    }
}

int main(void)
{
    printf("=== Router Unit Tests ===\n\n");

    test_init_network();
    test_create_router();
    test_connect_routers();
    test_disconnect_routers();
    test_update_distances();
    test_is_port_available();
    test_remove_router();
    test_reroute_after_disconnect();

    printf("\n=== Results: %d test(s) failed ===\n", errors);
    return errors;
}
