CC      := gcc
CFLAGS  := -O2 -g -Wall -Wextra -Wshadow -Wconversion -Wno-unused-parameter -std=gnu11 -Iinclude
LDFLAGS := 
TARGET  := forge

SRC := \
  src/main.c \
  src/master.c \
  src/worker.c \
  src/net.c \
  src/http/parser.c \
  src/http/static.c \
  src/util/log.c

OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
