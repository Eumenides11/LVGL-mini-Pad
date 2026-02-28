#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>       // 【新增】包含 open, O_RDONLY 等宏

#include "lvgl/lvgl.h"
#include "hal/hal.h"

// =======================================================
// 退出按钮的回调
// =======================================================
static void exit_btn_event_cb(lv_event_t * e) {
    (void)e;
    printf("[File Manager] 退出并释放资源...\n");
    exit(0); 
}

// =======================================================
// 【新增核心】列表项点击回调：使用 POSIX I/O 读取文件
// =======================================================
static void list_btn_event_cb(lv_event_t * e) {
    // 获取绑定的文件名 (删除了未使用的 btn 变量)
    const char * filename = (const char *)lv_event_get_user_data(e);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./%s", filename);

    struct stat file_stat;
    stat(filepath, &file_stat);

    if (S_ISDIR(file_stat.st_mode)) {
        printf("点击了目录: %s\n", filename);
        return;
    }

    printf("[底层I/O] 准备系统调用 open() 打开文件: %s\n", filepath);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("open 失败");
        return;
    }

    size_t read_size = (file_stat.st_size > 1024) ? 1024 : file_stat.st_size;
    char * buffer = (char *)malloc(read_size + 1);
    if (buffer == NULL) {
        close(fd);
        return;
    }

    ssize_t bytes_read = read(fd, buffer, read_size);
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0'; 
        printf("[底层I/O] 成功读取 %zd 字节。\n", bytes_read);
    } else {
        perror("read 失败");
        strcpy(buffer, "Error reading file.");
    }

    close(fd);

    // ==========================================
    // LVGL v9 适配写法：积木式拼接 MsgBox
    // ==========================================
    lv_obj_t * msgbox = lv_msgbox_create(lv_scr_act()); // 1. 创建空盒子
    lv_msgbox_add_title(msgbox, filename);              // 2. 加标题
    lv_msgbox_add_text(msgbox, buffer);                 // 3. 加文本内容
    lv_msgbox_add_close_button(msgbox);                 // 4. 加右上角关闭按钮 (自带点击关闭功能)
    lv_obj_center(msgbox);                              // 5. 居中显示

    free(buffer);
}

// =======================================================
// 【新增核心】解析文件权限 (类似 ls -l)
// 面试考点：位运算与 st_mode 标志位
// =======================================================
static void parse_permissions(mode_t mode, char * str) {
    strcpy(str, "---------");
    // 文件所有者权限
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    // 所在组权限
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    // 其他人权限
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

// =======================================================
// 读取目录并生成 UI 列表
// =======================================================
static void create_file_list_ui(void) {
    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xecf0f1), 0); 

    lv_obj_t * header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text_fmt(title, "File Manager (PID: %d)", getpid());
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);

    lv_obj_t * exit_btn = lv_btn_create(header);
    lv_obj_align(exit_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xe74c3c), 0); 
    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, LV_SYMBOL_CLOSE " Close");
    lv_obj_add_event_cb(exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * list = lv_list_create(screen);
    lv_obj_set_size(list, LV_PCT(90), LV_PCT(80));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);

    const char * scan_path = "."; 
    DIR * dir = opendir(scan_path);
    if (dir == NULL) return;

    struct dirent * entry;
    struct stat file_stat;
    char full_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // 略过隐藏文件

        snprintf(full_path, sizeof(full_path), "%s/%s", scan_path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) continue;

        // 【应用亮点】解析权限并展示
        char perm_str[10];
        parse_permissions(file_stat.st_mode, perm_str);

        const char * icon = S_ISDIR(file_stat.st_mode) ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;

        // 拼接展示字符串：图标 + 文件名 + 权限 + 大小
        char item_text[256];
        snprintf(item_text, sizeof(item_text), "%s | %s | %ld B", entry->d_name, perm_str, file_stat.st_size);
        
        // 创建列表按钮
        lv_obj_t * list_btn = lv_list_add_btn(list, icon, item_text);
        
        // 【关键】动态分配内存保存文件名，传给点击事件（防止指针失效）
        char * saved_name = strdup(entry->d_name); 
        lv_obj_add_event_cb(list_btn, list_btn_event_cb, LV_EVENT_CLICKED, (void *)saved_name);
    }
    
    closedir(dir); 
}

// =======================================================
// 主函数
// =======================================================
int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 

    lv_init();
    sdl_hal_init(800, 480);

    create_file_list_ui();

    printf("[File Manager] 初始化完毕，正在运行...\n");

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        usleep(sleep_time_ms * 1000);
    }

    return 0;
}