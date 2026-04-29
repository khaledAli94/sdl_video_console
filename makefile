CC      = gcc
CFLAGS  = -Wall -O2 -std=gnu99 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

TARGET = sdl_console

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
