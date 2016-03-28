CC=gcc
CFLAGS=-c -g -Wall -O3
SOURCES=20141589.c queue.c hexctrl.c filectrl.c
EXECUTABLE=20141589.out
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)
