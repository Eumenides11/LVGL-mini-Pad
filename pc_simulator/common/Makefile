CC = gcc
AR = ar

CFLAGS = -O3 -I. -I./lvgl -I./hal_sdl-Wall -I/usr/include/SDL2

# 自动获取源码，注意不要包含 examples，只编译核心 src
CSRCS = $(shell find lvgl/src -type f -name '*.c')
CSRCS += $(shell find hal_sdl -type f -name '*.c')

OBJS = $(CSRCS:.c=.o)
TARGET = liblvgl.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)