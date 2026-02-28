#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#ifndef _MSC_VER
#include <pthread.h>
#endif

/*-------------------------
 * 进程管理数据结构
 *------------------------*/
#define MAX_PROCESSES 20

typedef struct {
    pid_t pid;
    char name[32];
} ProcessInfo;

static ProcessInfo processes[MAX_PROCESSES];
static int process_count = 0;
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 添加新进程到列表 */
static void add_process(pid_t pid, const char *name) {
    pthread_mutex_lock(&process_mutex);
    if (process_count < MAX_PROCESSES) {
        processes[process_count].pid = pid;
        strncpy(processes[process_count].name, name, sizeof(processes[process_count].name)-1);
        processes[process_count].name[sizeof(processes[process_count].name)-1] = '\0';
        process_count++;
    }
    pthread_mutex_unlock(&process_mutex);
}

/* 从列表中移除指定 PID 的进程 */
static void remove_process(pid_t pid) {
    pthread_mutex_lock(&process_mutex);
    for (int i = 0; i < process_count; i++) {
        if (processes[i].pid == pid) {
            processes[i] = processes[process_count-1];
            process_count--;
            break;
        }
    }
    pthread_mutex_unlock(&process_mutex);
}

/*-------------------------
 * LVGL 界面组件
 *------------------------*/
static lv_obj_t *proc_table;   // 显示进程的表格

/* 刷新表格（定时器回调） */
static void refresh_table(lv_timer_t *timer) {
    (void)timer;

    // 回收所有已退出的子进程
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_process(pid);
    }

    // 更新表格
    lv_table_set_row_count(proc_table, 1);
    lv_table_set_cell_value(proc_table, 0, 0, "PID");
    lv_table_set_cell_value(proc_table, 0, 1, "Name");
    lv_table_set_cell_value(proc_table, 0, 2, "Status");
    lv_table_set_column_width(proc_table, 0, 80);
    lv_table_set_column_width(proc_table, 1, 120);
    lv_table_set_column_width(proc_table, 2, 80);

    pthread_mutex_lock(&process_mutex);
    for (int i = 0; i < process_count; i++) {
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", processes[i].pid);
        lv_table_set_cell_value(proc_table, i+1, 0, pid_str);
        lv_table_set_cell_value(proc_table, i+1, 1, processes[i].name);
        lv_table_set_cell_value(proc_table, i+1, 2, "Running");
    }
    pthread_mutex_unlock(&process_mutex);
}

/* “创建进程”按钮回调 */
static void create_btn_cb(lv_event_t *e) {
    (void)e;
    static int proc_counter = 1;

    pid_t pid = fork();
    if (pid == 0) {
        // 子进程：随机睡眠后退出
        srand(time(NULL) ^ (getpid() << 16));
        int sleep_time = 5 + rand() % 10;   // 5~14 秒
        sleep(sleep_time);
        exit(0);
    } else if (pid > 0) {
        char name[32];
        snprintf(name, sizeof(name), "Proc #%d", proc_counter++);
        add_process(pid, name);
    } else {
        perror("fork failed");
    }
}

/* 创建进程管理 UI */
void process_manager_create_ui(void) {
    // 标题
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Process Manager");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // 创建进程按钮
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 150, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(btn, create_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Create Process");
    lv_obj_center(btn_label);

    // 进程表格
    proc_table = lv_table_create(lv_scr_act());
    lv_obj_set_size(proc_table, 400, 300);
    lv_obj_align(proc_table, LV_ALIGN_TOP_MID, 0, 100);

    // 创建定时器，每 500ms 刷新一次表格
    lv_timer_t *timer = lv_timer_create(refresh_table, 500, NULL);
    lv_timer_ready(timer);
}
