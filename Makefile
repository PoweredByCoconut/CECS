CC = clang
#CFLAGS = -Iinclude -Wall -Wpedantic -g -O0
CFLAGS = -Iinclude -Wall -Wpedantic -O3
LDFLAGS = -lm -lwayland-client

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

open: $(TARGET)
	$(TARGET)
	viewnior test.ppm

test: $(TARGET)
	valgrind --leak-check=full $(TARGET)

clean: 
	rm out/*
