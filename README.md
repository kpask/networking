# Lab 1 — Multi-client guessing game server

Simple TCP server + client written in C. Server picks a random number (1-100), clients connect and try to guess it. First correct guess wins, all other clients are notified.

## Files

- `Lab1/server.c` — multi-client TCP server using `poll()`
- `Lab1/client.c` — interactive client with non-blocking input

## How to build

```bash
cd Lab1
gcc server.c -o server
gcc client.c -o client
```

## How to run

Start server first:

```bash
./server
```

Then connect one or more clients (use the port printed by server):

```bash
./client 127.0.0.1 [port]
```

## How it works

**Server:**

- Picks a random number on startup
- Uses `poll()` to handle multiple clients without threads or `fork()`
- `fds[0]` is always the listening socket, clients start from `fds[1]`
- When a client disconnects, their `fd` is set to `-1` so `poll()` ignores it
- When someone guesses correctly, notifies all other clients and exits

**Client:**

- Uses `poll()` on both `stdin` and the socket simultaneously
- Server messages (e.g. someone else won) are shown immediately, even while typing
- Prompt is shown only after receiving a server response

---

# Lab 2 — HTTP client

Minimal HTTP/1.0 client written in C. Connects to a server, sends a GET request, and saves the response body to a file. Works with any file type (HTML, JPG, PNG, etc).

## Files

- `Lab2/httpclient.c` — HTTP GET client

## How to build

```bash
cd Lab2
gcc httpclient.c -o httpclient
```

## How to run

```bash
./httpclient <url>
```

Examples:

```bash
./httpclient http://example.com/
./httpclient localhost:8080/file.png
```

## How it works

- Uses `getaddrinfo()` to resolve hostname to IP (DNS lookup)
- Builds a raw HTTP/1.1 GET request and sends it over a TCP socket
- Receives the full response into a dynamically growing buffer
- Locates the end of headers by searching for `\r\n\r\n`
- Writes everything after the headers (the body) to a file in binary mode

---

# Lab 3 — Distance Vector Routing Protocol

RIP-like routing protocol simulation written in C. Creates virtual routers that exchange distance vectors over UDP sockets, converge on shortest paths, detect link failures, and dynamically re-route.

## Files

- `Lab3/main.c` — interactive menu (create/connect/disconnect/print/send/remove routers)
- `Lab3/router.c` — router logic: Bellman-Ford, create/connect/disconnect/remove
- `Lab3/network.c` — UDP socket layer: broadcast, receive, timeout detection, packet simulation
- `Lab3/router.h` — data structures and function declarations
- `Lab3/network.h` — network layer function declarations
- `Lab3/test_router.c` — unit tests for router logic
- `Lab3/Makefile` — build targets

## How to build

```bash
cd Lab3
make
```

## How to run

```bash
./router_sim
```

## Menu options

1. **Create router** — assigns a UDP port and starts a router thread
2. **Connect routers** — creates a bidirectional link (cost = 1)
3. **Print routing table** — shows distances and next-hop for all active routers
4. **Disconnect routers** — removes a link between two routers
5. **Remove router** — stops the router thread and frees the slot
6. **Send packet** — simulates sending a packet through the network (follows next-hop)
7. **List active routers** — shows all running routers and their ports

## How it works

- Each router runs in its own `pthread` with a UDP socket bound to its port
- Every 2 seconds routers broadcast their distance vector to all neighbors
- On receiving an update, routers apply Bellman-Ford: if a shorter path is found through the neighbor, update `dist[]` and `next_hop[]`
- If no update is received from a neighbor for 6+ seconds, the link is marked dead and all routes through that neighbor become unreachable
- The network dynamically reconverges when links fail or are restored

## Unit tests

```bash
make test
```

Tests cover: initialization, router creation, port validation, connect/disconnect, distance vector learning, and router removal.
