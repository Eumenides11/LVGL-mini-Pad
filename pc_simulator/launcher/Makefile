CC = gcc

CFLAGS = -O3 -I../common -I../common/hal_sdl -Wall -I/usr/include/SDL2

# 关键：链接上一级的 liblvgl.a，以及系统的 SDL2 库
LDFLAGS = -L../common -llvgl -lSDL2 -lpthread -lm

CSRCS = src/main.c
OBJS = $(CSRCS:.c=.o)

TARGET = ../build/launcher

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p ../build
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)