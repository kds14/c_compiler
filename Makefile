CC=gcc
FLAGS=-g
BUILD=./build
TARGET=compiler

all:
	mkdir -p $(BUILD)
	$(CC) $(FLAGS) main.c -o $(BUILD)/$(TARGET)

clean:
	rm -r $(BUILD)
