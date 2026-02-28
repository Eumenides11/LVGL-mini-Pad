#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "hal_linux/hal.h" 

// =======================================================
// 全局变量区
// =======================================================
static lv_obj_t * label_time;          // 状态栏的时间标签
static lv_obj_t * label_cpu;           // 【新增】状态栏的 CPU 标签
static pthread_mutex_t lvgl_mutex;     // 多线程互斥锁



// =======================================================
// 【核心架构修改】应用启动事件：采用同步阻塞模式
// =======================================================
static void app_launch_event_cb(lv_event_t * e) {
    const char * app_path = (const char *)lv_event_get_user_data(e);
    printf("[桌面] 准备启动 App: %s\n", app_path);

    pid_t pid = fork();
    
    if (pid == 0) {
        // 【子进程】变身并接管屏幕
        execl(app_path, app_path, NULL);
        perror("[致命错误] execl 失败");
        exit(1);
    } 
    else if (pid > 0) {
        // 【父进程 (Launcher)】
        // 1. 陷入阻塞等待！这会导致 Launcher 的 LVGL 主循环暂停
        // 此时它不再往 fb0 刷数据，也不再读取触摸屏，完美让出硬件控制权！
        printf("[桌面] 已交出屏幕控制权，进入休眠，等待子进程 PID %d 结束...\n", pid);
        
        int status;
        waitpid(pid, &status, 0); // 0 表示阻塞死等

        // 2. 子进程退出了，父进程从这里苏醒！
        // 强制使当前屏幕对象失效，让 LVGL 在下一帧完全重绘桌面，覆盖掉 App 留下的残影
        printf("[桌面] 子进程结束，重新接管屏幕并刷新 UI！\n");
        lv_obj_invalidate(lv_scr_act());
    } 
    else {
        perror("fork 失败");
    }
}

// =======================================================
// 【新增核心模块】：解析 /proc/stat 计算 CPU 占用率
// =======================================================

static int get_cpu_usage(void) {
    // 静态变量保存上一次读取的 CPU 时间，因为 CPU 使用率是一个“时间差”的概念
    static unsigned long long prev_total = 0, prev_idle = 0;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long total, idle_calc, total_diff, idle_diff;
    int usage = 0;

    // 打开虚拟文件系统中的 stat 文件
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) return 0;

    char buffer[256];
    // 读取第一行（总 CPU 信息）
    if (fgets(buffer, sizeof(buffer), fp)) {
        // 解析前 8 个时间参数
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

        // 计算空闲时间 (idle + iowait)
        idle_calc = idle + iowait;
        // 计算总时间
        total = user + nice + system + idle_calc + irq + softirq + steal;

        // 如果不是第一次读取，就可以计算差值了
        if (prev_total != 0) {
            total_diff = total - prev_total;
            idle_diff = idle_calc - prev_idle;
            // CPU 使用率 = (总时间差 - 空闲时间差) / 总时间差 * 100
            if (total_diff > 0) {
                usage = (int)(100 * (total_diff - idle_diff) / total_diff);
            }
        }
        
        // 保存当前时间，供下一秒计算使用
        prev_total = total;
        prev_idle = idle_calc;
    }
    fclose(fp);
    return usage;
}

// =======================================================
// 模块 3：后台系统监控线程 (多线程核心)
// =======================================================
static void * sys_info_thread_func(void * arg) {
    (void)arg;
    char time_str[64];
    char cpu_str[32]; // 存放 CPU 文本
    time_t rawtime;
    struct tm * timeinfo;

    while(1) {
        // 1. 采集时间数据
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        sprintf(time_str, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        // 2. 【新增】采集 CPU 数据
        int cpu_usage = get_cpu_usage();
        sprintf(cpu_str, "CPU: %d%%", cpu_usage);

        // 3. 更新 UI（【关键】必须加锁）
        pthread_mutex_lock(&lvgl_mutex);
        if(label_time != NULL) {
            lv_label_set_text(label_time, time_str);
        }
        if(label_cpu != NULL) {
            lv_label_set_text(label_cpu, cpu_str); // 更新 CPU 标签
        }
        pthread_mutex_unlock(&lvgl_mutex);

        // 4. 睡眠 1 秒
        sleep(1);
    }
    return NULL;
}

// =======================================================
// 模块 4：构建桌面 UI
// =======================================================
static void create_desktop_ui(void) {
    lv_obj_t * desktop = lv_obj_create(lv_scr_act());
    lv_obj_set_size(desktop, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(desktop, lv_color_hex(0x1e272e), 0); 

    lv_obj_t * status_bar = lv_obj_create(desktop);
    lv_obj_set_size(status_bar, LV_PCT(100), 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x485460), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);

    // 右侧：时间标签
    label_time = lv_label_create(status_bar);
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_time, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(label_time, lv_color_hex(0xffffff), 0);

    // 【新增】左侧：CPU 占用率标签
    label_cpu = lv_label_create(status_bar);
    lv_label_set_text(label_cpu, "CPU: --%");
    lv_obj_align(label_cpu, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(label_cpu, lv_color_hex(0x00d2d3), 0); // 给 CPU 数据一个亮青色

    lv_obj_t * btn_fm = lv_btn_create(desktop);
    lv_obj_set_size(btn_fm, 120, 120);
    lv_obj_align(btn_fm, LV_ALIGN_CENTER, -225, 0);
    lv_obj_t * label_fm = lv_label_create(btn_fm);
    lv_label_set_text(label_fm, "File\nManager");
    lv_obj_center(label_fm);
    lv_obj_add_event_cb(btn_fm, app_launch_event_cb, LV_EVENT_CLICKED, (void *)"./build/file_manager");

    lv_obj_t * btn_sys = lv_btn_create(desktop);
    lv_obj_set_size(btn_sys, 120, 120);
    lv_obj_align(btn_sys, LV_ALIGN_CENTER, -75, 0);
    lv_obj_t * label_sys = lv_label_create(btn_sys);
    lv_label_set_text(label_sys, "System\nMonitor");
    lv_obj_center(label_sys);
    lv_obj_add_event_cb(btn_sys, app_launch_event_cb, LV_EVENT_CLICKED, (void *)"./build/sys_monitor");

    // ... 原有的 btn_sys 代码 ...

    // 5. 【新增】App 图标 (Button) - 相册
    lv_obj_t * btn_photo = lv_btn_create(desktop);
    lv_obj_set_size(btn_photo, 120, 120);
    // 把相册图标放在前两个图标的下方中心
    lv_obj_align(btn_photo, LV_ALIGN_CENTER, 75, 0); 
    lv_obj_t * label_photo = lv_label_create(btn_photo);
    lv_label_set_text(label_photo, "Photo\nViewer");
    lv_obj_center(label_photo);
    // 绑定点击事件，拉起 photo_viewer
    lv_obj_add_event_cb(btn_photo, app_launch_event_cb, LV_EVENT_CLICKED, (void *)"./build/photo_viewer");
  

    // 6. 【新增】App 图标 - 视频播放器
    lv_obj_t * btn_video = lv_btn_create(desktop);
    lv_obj_set_size(btn_video, 120, 120);
    lv_obj_align(btn_video, LV_ALIGN_CENTER, 225,0); // 位置放左下方
    lv_obj_t * label_video = lv_label_create(btn_video);
    lv_label_set_text(label_video, "Video\nPlayer");
    lv_obj_center(label_video);
    lv_obj_add_event_cb(btn_video, app_launch_event_cb, LV_EVENT_CLICKED, (void *)"./build/video_player");
}

// =======================================================
// 主函数
// =======================================================
int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 

    pthread_mutex_init(&lvgl_mutex, NULL);


    if (chdir("/root/lvgl_os") != 0) {
        perror("[致命错误] 找不到工作目录 /userdata/lvgl_os,请检查文件夹是否上传正确！");
    }


    lv_init();
    //sdl_hal_init(800, 480);
    linux_hal_init();

    create_desktop_ui();

    pthread_t sys_thread;
    pthread_create(&sys_thread, NULL, sys_info_thread_func, NULL);
    pthread_detach(sys_thread); 

    printf("[系统就绪] Launcher 启动成功！\n");

    while(1) {
        pthread_mutex_lock(&lvgl_mutex);
        uint32_t sleep_time_ms = lv_timer_handler();
        pthread_mutex_unlock(&lvgl_mutex);

        usleep(sleep_time_ms * 1000);
    }

    pthread_mutex_destroy(&lvgl_mutex);
    return 0;
}