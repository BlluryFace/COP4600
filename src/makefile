CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = rush

all: $(TARGET)

$(TARGET): rush.c
	$(CC) $(CFLAGS) rush.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.o