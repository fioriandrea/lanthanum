CC=gcc
TARGET=yasl
SOURCEDIR=src
SOURCES=$(wildcard src/*.c) $(wildcard src/*/*.c)
LFLAGS=-lm

OBJS=$(SOURCES:.c=.o)

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

cleanbuild:	purge all

clean:
	find $(SOURCEDIR) -type f -name '*.o' -print0 | xargs -0 rm -f
