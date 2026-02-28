# 引入交叉编译前缀。如果没传 CROSS_COMPILE，默认还是普通的 gcc 和 ar
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

# 剔除所有的 SDL 依赖，只保留纯 LVGL 的头文件路径和 -Wall 警告
CFLAGS = -O3 -I. -I./lvgl -Wall

# 自动获取源码，注意不要包含 examples，只编译 LVGL 核心 src
CSRCS = $(shell find lvgl/src -type f -name '*.c')
# (注：删除了 hal_sdl，因为我们已经在各个 App 的 Makefile 里独立编译 hal_linux.c 了)

OBJS = $(CSRCS:.c=.o)
TARGET = liblvgl.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)