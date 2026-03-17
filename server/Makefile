# Can run with "make CROSS_COMPILE=aarch64-none-linux-gnu-"

CC = gcc
CFLAGS = -g -Wall -Werror
TARGET = aesdsocket
SOURCES = aesdsocket.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(CROSS_COMPILE),)
    CC = gcc
else
    CC = $(CROSS_COMPILE)gcc
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
