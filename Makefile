CC = gcc
CFLAGS = -Wall -Wextra -std=c99 $(shell pkg-config --cflags sdl2 SDL2_ttf)
LIBS = $(shell pkg-config --libs sdl2 SDL2_ttf) -lm

SRCS = main.c particle.c
OBJS = $(SRCS:.c=.o)
TARGET = physics_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
