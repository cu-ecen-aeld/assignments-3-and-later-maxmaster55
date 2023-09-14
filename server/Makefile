CC = $(CROSS_COMPILE)gcc

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) aesdsocket.o -o aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) -c aesdsocket.c -o aesdsocket.o

.PHONY: clean
clean:
	rm -rf aesdsocket
	rm -rf *.o
