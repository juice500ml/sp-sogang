CC=gcc
CFLAGS=-c -Wall -O3
LDFLAGS=
SOURCES=20141589.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=20141589.out

all: $(SOURCES) $(EXECUTABLE)
	    
$(EXECUTABLE): $(OBJECTS) 
	    $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	    $(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o $(EXECUTABLE)
