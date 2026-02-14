# Can run with "make CROSS_COMPILE=aarch64-none-linux-gnu-"

CC = gcc
# CROSS_COMPILE = aarch64-none-linux-gnu-
TARGET = writer
SOURCES = writer.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(CROSS_COMPILE),)
	CC = gcc
else
	CC = $(CROSS_COMPILE)gcc
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
