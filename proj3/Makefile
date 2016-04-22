CC=gcc
CFLAGS=-c -Wall -O3
SOURCES=20141589.c queue.c hexctrl.c filectrl.c objctrl.c
EXECUTABLE=20141589.out
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.obj *.lst $(EXECUTABLE)
