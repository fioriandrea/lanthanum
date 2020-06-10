CC=gcc
TARGET=yaspl
SOURCEDIR=src
SOURCES=$(shell find $(SOURCEDIR) -name '*.c')
LFLAGS=-lm

OBJS=$(SOURCES:.c=.o)

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

clean:
	find $(SOURCEDIR) -type f -name '*.o' -print0 | xargs -0 rm -f
