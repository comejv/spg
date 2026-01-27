CC = gcc
CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lm -lraylib

.PHONY: clean

main: main.c geodesic.c
	$(CC) $(CFLAGS) -o main main.c geodesic.c $(LDFLAGS)

clean:
	rm -f main
