#include "lvgl/lvgl.h"
#include "hal_linux/hal.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/input.h>

#define VIDEO_DIR "./videos"

// =======================================================
// 退出 App
// =======================================================
static void exit_app_cb(lv_event_t * e) {
    printf("[媒体中心] 退出应用，交还屏幕控制权。\n");
    exit(0); 
}

// =======================================================
// 核心：调用 ffmpeg 暴力推流显存 + 触摸秒退监控
// =======================================================
static void play_video_event_cb(lv_event_t * e) {
    const char * filepath = (const char *)lv_event_get_user_data(e);
    
    printf("\n[媒体中心] -----------------------------------\n");
    printf("[媒体中心] 准备黑客级播放: %s\n", filepath);
    
    // 强制把背景刷黑
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_timer_handler(); 

    pid_t pid = fork();

    if (pid == 0) {
        // 【子进程：化身 ffmpeg 推流器】
        // 注意：如果你发现播放时画面偏色（比如人脸发蓝），请把下面的 "bgra" 换成 "rgb565le" 或 "rgba"
        execlp("ffmpeg", "ffmpeg", 
               "-re",                 // 必须加：按视频真实帧率播放，否则会像快进一样瞬间播完
               "-loglevel", "quiet",  // 保持控制台清净
               "-i", filepath,        // 输入视频文件
               "-an",                 // 必须加：静音播放，极大降低单核 CPU 负载
               "-vf", "scale=800:480",
               "-f", "fbdev",         // 强制输出为 Framebuffer 设备格式
               "-pix_fmt", "bgra",    // 像素格式转换 (RV1106 常用 bgra)
               "/dev/fb0",            // 目标输出：直接砸向物理屏幕！
               NULL);

        perror("[致命错误] 无法拉起 ffmpeg");
        exit(1);
    } 
    else if (pid > 0) {
        // 【父进程：化身监控者】
        int touch_fd = open("/dev/input/by-path/platform-ff470000.i2c-event", O_RDONLY | O_NONBLOCK);
        if (touch_fd < 0) {
            touch_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
        }

        struct input_event ev;
        int status;
        printf("[媒体中心] 正在全屏播放中... (点击屏幕任意位置强制退出)\n");

        while (1) {
            // 检查视频是否自然播完
            if (waitpid(pid, &status, WNOHANG) > 0) {
                printf("[媒体中心] 视频自然播放结束。\n");
                break; 
            }

            // 检查是否有人摸了屏幕
            if (touch_fd >= 0 && read(touch_fd, &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_KEY && ev.value == 1) { 
                    printf("\n[媒体中心] ⚡ 触屏拦截触发！一键击杀 ffmpeg (PID: %d)!\n", pid);
                    kill(pid, SIGKILL);       // 无情开火
                    waitpid(pid, &status, 0); // 收尸
                    break;
                }
            }
            usleep(50000); // 睡 50ms 再查
        }

        if (touch_fd >= 0) close(touch_fd);

        // 视频结束，重绘画布，恢复 UI 面貌
        printf("[媒体中心] 夺回屏幕控制权，恢复 UI 界面。\n");
        lv_obj_invalidate(lv_scr_act());
    } 
    else {
        perror("[错误] fork 失败");
    }
}

// =======================================================
// 构建 UI 界面
// =======================================================
static void create_video_list_ui(void) {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e1e), 0);

    lv_obj_t * header = lv_obj_create(scr);
    lv_obj_set_size(header, 800, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "Local Video Player");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t * exit_btn = lv_button_create(header);
    lv_obj_set_size(exit_btn, 80, 40);
    lv_obj_align(exit_btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xd32f2f), 0);
    lv_obj_add_event_cb(exit_btn, exit_app_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Close");
    lv_obj_set_style_text_color(exit_label, lv_color_white(), 0);
    lv_obj_center(exit_label);

    lv_obj_t * list_container = lv_obj_create(scr);
    lv_obj_set_size(list_container, 760, 380);
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x2b2b2b), 0);
    lv_obj_set_style_border_width(list_container, 0, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN); 
    lv_obj_set_style_pad_row(list_container, 10, 0); 

    DIR *dir = opendir(VIDEO_DIR);
    if (!dir) {
        lv_obj_t * err_label = lv_label_create(list_container);
        lv_label_set_text(err_label, "Error: Cannot open ./videos directory!");
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xff5555), 0);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".mp4") || strstr(entry->d_name, ".avi")) {
            char * fullpath = malloc(512);
            snprintf(fullpath, 512, "%s/%s", VIDEO_DIR, entry->d_name);

            lv_obj_t * item_btn = lv_button_create(list_container);
            lv_obj_set_size(item_btn, lv_pct(100), 60);
            lv_obj_set_style_bg_color(item_btn, lv_color_hex(0x444444), 0);
            lv_obj_add_event_cb(item_btn, play_video_event_cb, LV_EVENT_CLICKED, fullpath);

            lv_obj_t * item_label = lv_label_create(item_btn);
            lv_label_set_text_fmt(item_label, LV_SYMBOL_VIDEO "  %s", entry->d_name);
            lv_obj_set_style_text_color(item_label, lv_color_white(), 0);
            lv_obj_align(item_label, LV_ALIGN_LEFT_MID, 20, 0);
        }
    }
    closedir(dir);
}

// =======================================================
// 主函数入口
// =======================================================
int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 

    // 绑定工作目录
    if (chdir("/userdata/lvgl_os") != 0) {
        perror("[警告] 无法切换到 /userdata/lvgl_os");
    }

    lv_init();
    linux_hal_init();
    create_video_list_ui();

    printf("[系统] Video Player 启动成功！\n");

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        usleep(sleep_time_ms * 1000);
    }

    return 0;
}