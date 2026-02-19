CC = clang
CFLAGS = -Iinclude -Wall -Wpedantic
LDFLAGS = -lm

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

test: $(TARGET)
	valgrind --leak-check=full $(TARGET)
