# LVGL-mini-Pad
# ? RV1106 Multi-Process LVGL Desktop OS

![LVGL](https://img.shields.io/badge/LVGL-v9.x-blue.svg)
![Platform](https://img.shields.io/badge/Platform-RV1106%20%7C%20Ubuntu-green.svg)
![Build](https://img.shields.io/badge/Build-Makefile-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)



## ? 项目简介 (Introduction)

本项目是一个基于轻量级图形库 **LVGL** 开发的**多进程嵌入式桌面系统**。与传统的单进程 RTOS 图形界面不同，本项目采用了标准的 Linux 多进程架构（`fork` + `exec`），实现了一个包含桌面启动器（Launcher）和多个独立子应用（文件管理器、视频播放器、系统监控等）的完整生态。

项目针对 **瑞芯微 RV1106 (ARM Cortex-A7)** 进行了深度的底层硬件适配，抛弃了臃肿的 X11/Wayland 桌面环境依赖，直接操作 Linux 原生 Framebuffer 与输入子系统，实现了极低资源消耗下的流畅交互。同时保留了基于 SDL2 的 PC 仿真环境，方便跨平台敏捷开发。

---

## ? 核心特性 (Key Features)

* **?? 真正的多进程隔离调度**
  * 采用 `fork()` + `exec()` 拉起独立子应用进程。
  * Launcher 采用同步阻塞 (`waitpid`) 机制，启动 App 后自动挂起，完美让出 `/dev/fb0` 显存与底层触摸控制权，App 退出后瞬间接管并重绘桌面。
* **?? 极致轻量的原生硬件驱动**
  * **显示**：直写 `/dev/fb0` 显存，无视图形界面服务器的开销。
  * **输入**：动态绑定 `/dev/input/by-path/platform-ff470000.i2c-event` 或 `/dev/input/event0`，直连物理电信号。
* **? 极客级“降维打击”视频播放器**
  * 针对单片机缺乏窗口管理器的痛点，摒弃常规播放器 API，利用 `ffmpeg` 命令提取原始帧并强制转换为 `bgra` 像素格式，暴力推流直写物理屏幕。
  * **硬件级秒退机制**：播放视频时，父进程通过非阻塞 (`O_NONBLOCK`) 方式监听触摸屏中断电信号。用户触屏瞬间触发 `SIGKILL` 击杀解码子进程，无缝退回桌面。
* **? 全局路径防抖设计**
  * 入口处强制绑定绝对工作路径 (`chdir`)，彻底解决嵌入式设备开机自启或绝对路径调用时产生的资源文件（图片、视频）丢失问题。

---

## ? 目录结构 (Repository Structure)

```text
RV1106_LVGL_Desktop/
├── pc_simulator/          # ? PC 端 Ubuntu 仿真环境源码 (基于 SDL2)
├── rv1106_board/          # ? RV1106 ARM 交叉编译源码 (适配 Linux HAL 原生驱动)
├── release_to_board/      # ? 目标板开箱即用部署包 (含可执行文件、测试图片、视频)
├── docs/                  # ? 开发手册与移植实战踩坑指南
└── README.md              # ? 项目说明文档
```
---

## ? 效果展示
* 仿真效果：
<img width="600" height="526" alt="637143caa942e072bc058780f6510194" src="https://github.com/user-attachments/assets/9341056f-7775-44b6-ae7a-92aa6ad9f1e6" />
<img width="600" height="515" alt="25149e902c48934fbac644a5e6e6e47f" src="https://github.com/user-attachments/assets/97c45807-6698-4340-9aff-2859a366d2af" />
<img width="600" height="519" alt="4feaaf3478a738696a80955ab2582886" src="https://github.com/user-attachments/assets/7a5370f5-47b8-4e8f-a301-b780c2f1862c" />


* 实机效果：
* ![c3e5cd22c37a720076345a161bdfe8b2](https://github.com/user-attachments/assets/8b71ca7d-7c5d-482a-8bba-90fe51e19e41)
* ![598abbdeb4f105d2a69d886c44d16545](https://github.com/user-attachments/assets/068ed64b-276e-4c86-a33c-aebb88171191)



