#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "hal/hal.h"

// =======================================================
// 事件回调
// =======================================================
static void exit_btn_event_cb(lv_event_t * e) {
    (void)e;
    printf("[Video Player] 退出应用...\n");
    exit(0); 
}

// 【核心机制】点击列表中的视频文件，拉起外部播放器
static void play_video_event_cb(lv_event_t * e) {
    const char * filename = (const char *)lv_event_get_user_data(e);
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./videos/%s", filename);

    printf("[媒体中心] 准备调用外部解码器播放视频: %s\n", filepath);

    // 构建命令行指令
    // ffplay: 播放器
    // -autoexit: 播完自动退出，把控制权还给我们的程序
    // -x 800 -y 480: 指定窗口大小（模拟全屏）
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ffplay -autoexit -x 800 -y 480 \"%s\"", filepath);

    // 【面试重点】使用 system() 阻塞式拉起子进程。
    // 在这期间，LVGL 会暂停渲染，控制权完全交给 ffplay 视频播放器。
    // 播完或用户关闭播放窗口后，system() 返回，LVGL 恢复生机！
    int ret = system(cmd);
    
    if (ret == -1) {
        perror("[错误] 无法拉起播放器，请检查是否安装了 ffmpeg");
    } else {
        printf("[媒体中心] 视频播放结束，UI 重新接管控制权。\n");
    }
}

// =======================================================
// 构建 UI (类似文件管理器，使用 List 列表控件)
// =======================================================
static void create_video_ui(void) {
    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x2c3e50), 0); // 深蓝底色

    // 1. 标题栏
    lv_obj_t * header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xe67e22), 0); // 橙色主题
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "Media Center - Video Player");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);

    lv_obj_t * exit_btn = lv_btn_create(header);
    lv_obj_align(exit_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xc0392b), 0);
    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, LV_SYMBOL_CLOSE " Close");
    lv_obj_add_event_cb(exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 2. 视频列表容器
    lv_obj_t * list = lv_list_create(screen);
    lv_obj_set_size(list, LV_PCT(90), LV_PCT(80));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);

    // 3. 扫描 ./videos 目录
    DIR * dir = opendir("./videos");
    if (dir != NULL) {
        struct dirent * entry;
        while ((entry = readdir(dir)) != NULL) {
            // 只过滤 .mp4 或 .avi 文件
            if (strstr(entry->d_name, ".mp4") != NULL || strstr(entry->d_name, ".avi") != NULL) {
                
                // 深拷贝文件名给点击事件
                char * saved_name = strdup(entry->d_name);
                
                // 列表项带有播放图标 (PLAY)
                lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_PLAY, entry->d_name);
                lv_obj_add_event_cb(btn, play_video_event_cb, LV_EVENT_CLICKED, (void *)saved_name);
            }
        }
        closedir(dir);
    } else {
        lv_list_add_text(list, "Folder './videos' not found or empty.");
    }
}

int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 

    lv_init();
    sdl_hal_init(800, 480);
    create_video_ui();
    
    printf("[Video Player] 初始化完毕...\n");

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        usleep(sleep_time_ms * 1000);
    }
    return 0;
}