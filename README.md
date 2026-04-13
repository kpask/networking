# Lab 1 — Multi-client guessing game server

Simple TCP server + client written in C. Server picks a random number (1-100), clients connect and try to guess it. First correct guess wins, all other clients are notified.

## Files

- `server.c` — multi-client TCP server using `poll()`
- `client.c` — interactive client with non-blocking input

## How to build

gcc server.c -o server
gcc client.c -o client

## How to run

Start server first:
./server

Then connect one or more clients (use the port printed by server):
./client 127.0.0.1 [port]

## How it works

**Server:**

- Picks a random number on startup
- Uses `poll()` to handle multiple clients without threads or `fork()`
- `fds[0]` is always the listening socket, clients start from `fds[1]`
- When a client disconnects, their `fd` is set to `-1` so `poll()` ignores it
- When someone guesses correctly, notifies all other clients and exits

**Client:**

- Uses `poll()` on both `stdin` and the socket simultaneously
- This means server messages (e.g. someone else won) are shown immediately, even while the user is typing
- Prompt is shown only after receiving a server response
