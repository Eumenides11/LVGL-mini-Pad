#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *getenv_default(const char *name, const char *dflt) {
    const char *val = getenv(name);
    return val ? val : dflt;
}

void linux_hal_init(void) {
    // 1. 挂载 Framebuffer
    const char * fb_dev = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, fb_dev);
    printf("[HAL] 显示驱动挂载: %s\n", fb_dev);

    // 2. 挂载触摸屏 (优先使用官方绝对物理路径，防漂移)
    const char * touch_dev = "/dev/input/by-path/platform-ff470000.i2c-event";
    if (access(touch_dev, F_OK) != 0) {
        touch_dev = "/dev/input/event0"; // 降级方案
    }

    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_dev);
    if(indev) {
        lv_indev_set_display(indev, disp);
        printf("[HAL] 触摸驱动挂载: %s\n", touch_dev);
    }
}