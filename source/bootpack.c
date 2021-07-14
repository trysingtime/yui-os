#include "bootpack.h"
#include <stdio.h>

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256];
    int mx, my;

    init_palette(); // 设定调色盘
    init_screen8(bootinfo -> vram, bootinfo -> screenx, bootinfo -> screeny); // 初始化屏幕

    init_gdtidt(); // 初始化GDT/IDT
    init_pic(); // 初始化PIC
    io_sti(); // 允许中断

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

    // 开放PIC, 键盘是IRQ1, PIC1是IRQ2, 鼠标是IRQ12
	io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

    // 待机
    for (;;) {
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}
