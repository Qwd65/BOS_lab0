CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = file_ops

$(TARGET): file_ops.o
	$(CC) $(CFLAGS) -o $(TARGET) file_ops.o

file_ops.o: file_ops.c
	$(CC) $(CFLAGS) -c file_ops.c

clean:
	rm -f $(TARGET) file_ops.o
