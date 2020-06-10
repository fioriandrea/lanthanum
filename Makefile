CC=gcc
TARGET=yaspl
SOURCEDIR=./
SOURCES=$(shell find $(SOURCEDIR) -name '*.c')
LFLAGS=-lm

OBJS=$(SOURCES:.c=.o)

# the target is obtained linking all .o files
all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

clean:
	find $(SOURCEDIR) -type f -name '*.o' -print0 | xargs -0 rm -f
