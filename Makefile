CC = clang
CFLAGS = -Iinclude -Wall -Wpedantic -std=c11
LDFLAGS = -lm -lncurses

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

host: $(TARGET)
	$(TARGET) hoster

client: $(TARGET)
	$(TARGET) client

test: $(TARGET)
	valgrind --leak-check=full $(TARGET)

clean:
	rm $(OBJ) $(TARGET)
