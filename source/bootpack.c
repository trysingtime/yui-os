#include "bootpack.h"
#include <stdio.h>

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256];
    int mx, my;

    init_palette(); // 设定调色盘
    init_screen8(bootinfo -> vram, bootinfo -> screenx, bootinfo -> screeny); // 初始化屏幕

    // 绘制鼠标指针
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(bootinfo -> vram, bootinfo -> screenx, 16, 16, mx, my, mcursor, 16);

    // 绘制字符串
 	putfonts8_asc(bootinfo -> vram, bootinfo -> screenx,  8,  8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc(bootinfo -> vram, bootinfo -> screenx, 31, 31, COL8_000000, "Haribote OS."); // 文字阴影效果
	putfonts8_asc(bootinfo -> vram, bootinfo -> screenx, 30, 30, COL8_FFFFFF, "Haribote OS.");

    // 绘制变量
    sprintf(s, "screenx = %d", bootinfo -> screenx);
    putfonts8_asc(bootinfo -> vram, bootinfo -> screenx, 16, 64, COL8_FFFFFF, s);

    // 待机
    for (;;) {
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}
