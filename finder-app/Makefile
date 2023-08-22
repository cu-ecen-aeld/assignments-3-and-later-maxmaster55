CC=gcc
CFLAGS= -Wall
name=writer
CROSS_COMPILE?=""

all: $(name)
	$(CROSS_COMPILE)$(CC) *.c -o $(name)

clean:
	rm -f *.o $(name)
