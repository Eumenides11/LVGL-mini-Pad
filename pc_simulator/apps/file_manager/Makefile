# apps/file_manager/Makefile
CC = gcc
# 注意这里的路径：由于深了一层，头文件和库的路径变成了 ../../common
CFLAGS = -O3 -I../../common -I../../common/hal_sdl -Wall -I/usr/include/SDL2
LDFLAGS = -L../../common -llvgl -lSDL2 -lpthread -lm

CSRCS = src/main.c
OBJS = $(CSRCS:.c=.o)
# 编译出来的可执行文件直接放到根目录的 build 文件夹下
TARGET = ../../build/file_manager

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p ../../build
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)