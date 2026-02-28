#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "hal/hal.h"

// 全局 UI 控件指针，方便定时器更新
static lv_obj_t * label_mem_details;
static lv_obj_t * bar_mem;
static lv_obj_t * label_mem_pct;

// =======================================================
// 退出按钮的回调
// =======================================================
static void exit_btn_event_cb(lv_event_t * e) {
    (void)e;
    printf("[Sys Monitor] 用户点击退出，子进程结束。\n");
    exit(0); // 自杀，触发 Launcher 的 waitpid 回收
}

// =======================================================
// 【核心底层逻辑】解析 /proc/meminfo
// =======================================================
static void update_memory_data(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("无法打开 /proc/meminfo");
        return;
    }

    unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
    char buffer[256];

    // 逐行读取并解析需要的字段 (单位是 kB)
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "MemTotal:", 9) == 0) {
            sscanf(buffer, "MemTotal: %lu kB", &mem_total);
        } else if (strncmp(buffer, "MemFree:", 8) == 0) {
            sscanf(buffer, "MemFree: %lu kB", &mem_free);
        } else if (strncmp(buffer, "MemAvailable:", 13) == 0) {
            sscanf(buffer, "MemAvailable: %lu kB", &mem_available);
        }
    }
    fclose(fp);

    if (mem_total > 0) {
        // 计算已用内存 (Linux 真实可用内存看 MemAvailable 更准确)
        unsigned long mem_used = mem_total - mem_available;
        int usage_pct = (int)((mem_used * 100) / mem_total);

        // 转换为 MB
        unsigned long total_mb = mem_total / 1024;
        unsigned long used_mb = mem_used / 1024;
        unsigned long avail_mb = mem_available / 1024;

        // 更新 UI 上的文本
        lv_label_set_text_fmt(label_mem_details, 
            "Total Memory: %lu MB\n"
            "Used Memory: %lu MB\n"
            "Available: %lu MB", 
            total_mb, used_mb, avail_mb);

        // 更新 UI 上的进度条和百分比
        lv_bar_set_value(bar_mem, usage_pct, LV_ANIM_ON);
        lv_label_set_text_fmt(label_mem_pct, "%d %%", usage_pct);
    }
}

// =======================================================
// LVGL 定时器回调 (每秒执行一次)
// =======================================================
static void mem_timer_cb(lv_timer_t * timer) {
    (void)timer;
    update_memory_data(); // 刷新内存数据
}

// =======================================================
// 构建 UI
// =======================================================
static void create_sys_monitor_ui(void) {
    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x2d3436), 0); // 深灰背景

    // 1. 标题栏
    lv_obj_t * header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x00b894), 0); // 绿色主题
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text_fmt(title, "System Monitor (PID: %d)", getpid());
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);

    // 2. 退出按钮
    lv_obj_t * exit_btn = lv_btn_create(header);
    lv_obj_align(exit_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xd63031), 0); 
    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, LV_SYMBOL_CLOSE " Close");
    lv_obj_add_event_cb(exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 3. 内存使用率进度条 (Bar)
    bar_mem = lv_bar_create(screen);
    lv_obj_set_size(bar_mem, 400, 30);
    lv_obj_align(bar_mem, LV_ALIGN_CENTER, 0, -30);
    lv_bar_set_range(bar_mem, 0, 100);
    
    // 进度条上的百分比文字
    label_mem_pct = lv_label_create(screen);
    lv_obj_align_to(label_mem_pct, bar_mem, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
    lv_obj_set_style_text_color(label_mem_pct, lv_color_hex(0xffffff), 0);

    // 4. 详细数据文本
    label_mem_details = lv_label_create(screen);
    lv_obj_align(label_mem_details, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(label_mem_details, lv_color_hex(0xffffff), 0);
    // 设置行间距
    lv_obj_set_style_text_line_space(label_mem_details, 10, 0);

    // 5. 首次获取数据并启动 LVGL 定时器 (每 1000ms 触发一次)
    update_memory_data();
    lv_timer_create(mem_timer_cb, 1000, NULL);
}

// =======================================================
// 主函数
// =======================================================
int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 

    lv_init();
    sdl_hal_init(800, 480);

    create_sys_monitor_ui();

    printf("[Sys Monitor] 初始化完毕，正在运行...\n");

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        usleep(sleep_time_ms * 1000);
    }

    return 0;
}