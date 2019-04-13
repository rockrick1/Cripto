CC=gcc
CFLAGS=-Wall -pedantic -O2
OBJECTS=ep.c

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

ep: $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o ep

clean:
	rm -rf *.o