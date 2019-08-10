CC=gcc
FLAGS=-g
BUILD=./build
TARGET=compiler
SRC=*.c

all:
	mkdir -p $(BUILD)
	$(CC) $(FLAGS) $(SRC) -o $(BUILD)/$(TARGET)

clean:
	rm -r $(BUILD)
