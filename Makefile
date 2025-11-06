COMPILER = gcc

CFLAGS = -std=c99

#more flags: -Wall -Wextra

TARGET = popkernels

all: $(TARGET)

$(TARGET): Popcorn.c
	$(COMPILER) $(CFLAGS) Popcorn.c -o $(TARGET)

clean:
	rm -f $(TARGET)