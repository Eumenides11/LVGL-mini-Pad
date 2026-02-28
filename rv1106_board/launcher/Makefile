CC = $(CROSS_COMPILE)gcc

CFLAGS = -O3 -I../common -I../common/hal_linux -Wall 

# 关键：链接上一级的 liblvgl.a，以及系统的 SDL2 库
LDFLAGS = -L../common -llvgl -lpthread -lm

CSRCS = src/main.c ../common/hal_linux/hal.c
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