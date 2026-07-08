CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
SRC = src/thread_pool.c examples/main.c
OUT = pool_example

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)

.PHONY: all run clean
