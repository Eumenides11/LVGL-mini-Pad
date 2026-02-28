.PHONY: all common launcher file_manager sys_monitor photo_viewer  video_player clean

all: common launcher file_manager sys_monitor photo_viewer video_player
	@echo "编译成功! 所有的可执行文件都在 build/ 目录下"

common:
	$(MAKE) -C common

launcher: common
	$(MAKE) -C launcher

file_manager: common
	$(MAKE) -C apps/file_manager

sys_monitor: common
	$(MAKE) -C apps/sys_monitor

photo_viewer: common
	$(MAKE) -C apps/photo_viewer

video_player: common
	$(MAKE) -C apps/video_player

clean:
	$(MAKE) -C common clean
	$(MAKE) -C launcher clean
	$(MAKE) -C apps/file_manager clean
	$(MAKE) -C apps/sys_monitor clean
	$(MAKE) -C apps/photo_viewer clean
	$(MAKE) -C apps/video_player clean
	rm -rf build/