CC = gcc
CFLAGS = -Wall -Iinclude

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = main

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o main
