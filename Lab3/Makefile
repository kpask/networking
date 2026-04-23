CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = router_sim
SRCS = main.c router.c network.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) test_router test_router.o

run: $(TARGET)
	./$(TARGET)

test: test_router
	./test_router

test_router: test_router.o router.o
	$(CC) $(CFLAGS) -o $@ $^
