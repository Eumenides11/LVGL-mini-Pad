# 1. 引入交叉编译前缀，默认留空兼容本地编译
CC = $(CROSS_COMPILE)gcc

# 2. 删掉原来的 -I/usr/include/SDL2，更新 hal 路径
CFLAGS = -O3 -I../../common -I../../common/hal_linux -Wall 

# 3. 删掉原来的 -lSDL2，保留核心库
LDFLAGS = -L../../common -llvgl -lpthread -lm

# 4. 把底层的编译源文件换成我们刚写的 hal_linux/hal.c
CSRCS = src/main.c ../../common/hal_linux/hal.c
OBJS = $(CSRCS:.c=.o)

# 这里的 TARGET 名字各自保留原来的，比如 launcher 就是 ../../build/launcher
TARGET = ../../build/photo_viewer

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p ../../build
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)CC = $(CROSS_COMPILE)gcc

