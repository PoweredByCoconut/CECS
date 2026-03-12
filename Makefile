CC = clang
CFLAGS = -Iinclude -Wall -Wpedantic -g -O0
LDFLAGS = -lm -lSDL3

SRC = $(shell find src -name *.c)
OBJ = $(patsubst src/%.c, out/%.o, $(SRC))
TARGET = out/cecs

out/%.o: src/%.c
	mkdir -p out
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	echo $(OBJ)
	mkdir -p out
	$(CC) $(LDFLAGS) $^ -o $@

run: $(TARGET)
	$(TARGET)

test.ppm: $(TARGET)
	$(TARGET)

open: test.ppm
	viewnior test.ppm

test: $(TARGET)
	valgrind --leak-check=full $(TARGET)
