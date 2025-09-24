CC=gcc
CFLAGS=-Wall -Iinclude
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
TARGET=build/webserver

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f src/*.o $(TARGET)


