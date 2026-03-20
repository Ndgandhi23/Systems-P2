CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm

TARGET = compare
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

san:
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(SRCS) -o $(TARGET) $(LDFLAGS)

.PHONY: clean san