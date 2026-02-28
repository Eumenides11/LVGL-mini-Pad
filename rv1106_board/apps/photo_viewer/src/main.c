#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "hal_linux/hal.h"

static lv_obj_t * cont_gallery;   
static lv_obj_t * cont_fullview;  
static lv_obj_t * full_img_obj;   

// =======================================================
// 事件回调
// =======================================================
static void exit_btn_event_cb(lv_event_t * e) {
    (void)e;
    printf("[Photo Viewer] 退出应用，释放内存。\n");
    exit(0); 
}

static void back_btn_event_cb(lv_event_t * e) {
    (void)e;
    lv_obj_add_flag(cont_fullview, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cont_gallery, LV_OBJ_FLAG_HIDDEN);
    // 可选优化：返回时清空大图的内存占用
    lv_image_set_src(full_img_obj, NULL);
}

static void thumb_click_event_cb(lv_event_t * e) {
    // 【关键】这里传过来的是原始大图的路径！
    const char * orig_img_path = (const char *)lv_event_get_user_data(e);
    
    printf("[Photo Viewer] 用户点击，正在加载高清大图: %s\n", orig_img_path);

    // 让大图控件加载原始高清文件
    lv_image_set_src(full_img_obj, orig_img_path);

    lv_obj_add_flag(cont_gallery, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cont_fullview, LV_OBJ_FLAG_HIDDEN);
}

// =======================================================
// 核心 UI 构建与缩略图引擎
// =======================================================
static void create_photo_ui(void) {
    lv_obj_t * screen = lv_scr_act();

    // ---------------------------------------------------
    // 1. 构建大图页面 (隐藏)
    // ---------------------------------------------------
    cont_fullview = lv_obj_create(screen);
    lv_obj_set_size(cont_fullview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont_fullview, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont_fullview, 0, 0);
    lv_obj_set_style_radius(cont_fullview, 0, 0);
    lv_obj_add_flag(cont_fullview, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * back_btn = lv_btn_create(cont_fullview);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x7f8c8d), 0);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    full_img_obj = lv_image_create(cont_fullview);
    lv_obj_align(full_img_obj, LV_ALIGN_CENTER, 0, 0);

// ---------------------------------------------------
    // 【核心修复】将返回按钮强制移动到当前父容器的最前层 (Foreground)
    // 这样不管中间显示的图片多大，返回键永远悬浮在图片之上。
    // ---------------------------------------------------
    lv_obj_move_foreground(back_btn);
    // ---------------------------------------------------
    // 2. 构建照片墙页面
    // ---------------------------------------------------
    cont_gallery = lv_obj_create(screen);
    lv_obj_set_size(cont_gallery, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont_gallery, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont_gallery, 0, 0);
    lv_obj_set_style_radius(cont_gallery, 0, 0);

    lv_obj_t * header = lv_obj_create(cont_gallery);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x9b59b6), 0);
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "Enterprise Photo Gallery");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);

    lv_obj_t * exit_btn = lv_btn_create(header);
    lv_obj_align(exit_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // ... [保留你代码中上面的 header 创建部分] ...

    // ---------------------------------------------------
    // 修复：使用绝对像素严格控制网格大小与起点位置
    // ---------------------------------------------------
    lv_obj_t * grid = lv_obj_create(cont_gallery);
    
    // 宽度 800，高度 = 屏幕总高 480 - 标题栏 50 = 430
    lv_obj_set_size(grid, 800, 430);
    // 核心定位：紧贴屏幕左边缘 (0)，向下偏移 50 像素（紧贴在标题栏正下方）
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 50);

    // 消除默认内边距，设置图片间的缝隙为 2px
    lv_obj_set_style_pad_all(grid, 2, 0);       // 容器外圈留 2px
    lv_obj_set_style_pad_row(grid, 2, 0);       // 行间距 2px
    lv_obj_set_style_pad_column(grid, 2, 0);    // 列间距 2px
    lv_obj_set_style_bg_color(grid, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    // 启用 Flex 弹性布局
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    // 允许从左到右排布，放不下自动换行
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    // 强制三种对齐全部从起点（左侧/上侧）开始
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // ... [保留你代码中下面的 DIR 扫描本地图片代码] ...

    // ---------------------------------------------------
    // 3. 【核心高能】企业级缩略图引擎 (Thumbnail Engine)
    // ---------------------------------------------------
    // 检查并创建隐藏的缓存文件夹 ./photos/.thumbs
    struct stat st = {0};
    if (stat("./photos/.thumbs", &st) == -1) {
        printf("[系统] 缩略图缓存目录不存在，正在创建 ./photos/.thumbs\n");
        mkdir("./photos/.thumbs", 0777); // POSIX 创建目录 API
    }

    DIR * dir = opendir("./photos");
    if (dir != NULL) {
        struct dirent * entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".bmp") != NULL || strstr(entry->d_name, ".BMP") != NULL) {
                
                // 定义原始文件路径和对应的缩略图路径
                char linux_orig_path[256];
                char linux_thumb_path[256];
                snprintf(linux_orig_path, sizeof(linux_orig_path), "./photos/%s", entry->d_name);
                snprintf(linux_thumb_path, sizeof(linux_thumb_path), "./photos/.thumbs/%s", entry->d_name);

                // 【面试考点】使用 access() 检查文件是否存在
                // F_OK 标志用来测试文件存不存在
                if (access(linux_thumb_path, F_OK) != 0) {
                    printf("[引擎] 发现新图片 %s，正在后台生成缩略图...\n", entry->d_name);
                    
                    // 构建转换命令：调用 ImageMagick 生成强制 196x140 尺寸的小图
                    char cmd[512];
                    snprintf(cmd, sizeof(cmd), "convert \"%s\" -resize 196x140\\! \"%s\"", linux_orig_path, linux_thumb_path);
                    
                    // system() 底层就是 fork + exec + waitpid，自动帮我们开子进程去干活！
                    system(cmd); 
                    printf("[引擎] 缩略图生成完毕！\n");
                }

                // 准备传给 LVGL 的虚拟盘符路径
                char lvgl_orig_path[256];
                char lvgl_thumb_path[256];
                snprintf(lvgl_orig_path, sizeof(lvgl_orig_path), "A:%s", linux_orig_path);
                snprintf(lvgl_thumb_path, sizeof(lvgl_thumb_path), "A:%s", linux_thumb_path);

                // 深拷贝一份原始大图路径，留给点击事件用
                char * saved_orig_path = strdup(lvgl_orig_path);

                // 创建图片控件
                lv_obj_t * img = lv_image_create(grid);
                // 【性能飞跃】给照片墙加载的是小巧的缩略图 (几十KB)！
                lv_image_set_src(img, lvgl_thumb_path);
                
                // 设置控件大小，无需再进行拉伸渲染消耗 CPU
                lv_obj_set_size(img, 196, 140);
                
                lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
                // 绑定点击事件，把“大图路径”传过去
                lv_obj_add_event_cb(img, thumb_click_event_cb, LV_EVENT_CLICKED, (void *)saved_orig_path);
            }
        }
        closedir(dir);
    }
}

int main(int argc, char **argv) {
    (void)argc; 
    (void)argv; 



    if (chdir("/root/lvgl_os") != 0) {
        perror("[致命错误] 找不到工作目录 /userdata/lvgl_os，请检查文件夹是否上传正确！");
    }


    lv_init();
    //sdl_hal_init(800, 480);
    linux_hal_init();
    create_photo_ui();
    
    printf("[Photo Viewer] 媒体引擎扫描完毕，UI 已就绪。\n");

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        usleep(sleep_time_ms * 1000);
    }
    return 0;
}