CC=gcc
CFLAGS=-Wall -pedantic -O2 -lm
OBJECTS=ep.c opes.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<
	
all: ep clean

ep: $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o ep

clean:
	rm -rf *.o