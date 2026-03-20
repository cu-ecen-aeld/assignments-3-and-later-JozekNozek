CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
SOURCES ?= aesdsocket.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(CROSS_COMPILE),)
    CC ?= gcc
else
    CC ?= $(CROSS_COMPILE)gcc
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJECTS) -o $(TARGET)

aesdsocket.o: aesdsocket.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
