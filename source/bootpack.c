#include "bootpack.h"
#include <stdio.h>

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    struct MOUSE_DEC mdec;

    init_gdtidt(); // 初始化GDT/IDT
    init_pic(); // 初始化PIC
    io_sti(); // 允许中断

    fifo8_init(&keyfifo, 32, keybuf); // 初始化键盘缓冲区
    fifo8_init(&mousefifo, 128, mousebuf); // 初始化鼠标缓冲区
    // 开放PIC, 键盘是IRQ1, PIC1是IRQ2, 鼠标是IRQ12
	io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

    init_keyboard(); // 初始化键盘控制电路(包含鼠标控制电路)

    init_palette(); // 设定调色盘
    init_screen8(bootinfo -> vram, bootinfo -> screenx, bootinfo -> screeny); // 初始化屏幕
    // 绘制鼠标指针
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(bootinfo -> vram, bootinfo -> screenx, 16, 16, mx, my, mcursor, 16);
    // 绘制鼠标坐标
    sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 0, COL8_FFFFFF, s);
    // 绘制字符串
 	// putfonts8_asc(bootinfo->vram, bootinfo->screenx,  8,  8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 31, 31, COL8_000000, "Haribote OS."); // 文字阴影效果
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 30, 30, COL8_FFFFFF, "Haribote OS.");

    enable_mouse(&mdec); // 启用鼠标本身

    for (;;) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
        } else {
            if (fifo8_status(&keyfifo) != 0 ) {
                i = fifo8_get(&keyfifo);
                io_sti();

                boxfill8(bootinfo->vram, bootinfo->screenx, COL8_008484, 0, 16, 15, 31);
                sprintf(s, "%02X", i);
                putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();

                if (mouse_decode(&mdec, i) != 0) {
                    // 鼠标3字节已完整, 显示鼠标数值
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    // mdec.btn第0位: 左键, 第1位: 右键, 第2位:中建
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(bootinfo->vram, bootinfo->screenx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(bootinfo->vram, bootinfo->screenx, 32, 16, COL8_FFFFFF, s);

                    // 显示鼠标指针移动
                    boxfill8(bootinfo -> vram, bootinfo -> screenx, COL8_008484, mx, my, mx + 15, my + 15); // 隐藏鼠标(绘制背景色矩形遮住之前绘制好的鼠标)
                    boxfill8(bootinfo -> vram, bootinfo -> screenx, COL8_008484, 0, 0, 79, 15); // 隐藏坐标(绘制背景色矩形遮住之前绘制好的坐标)
                    // 计算鼠标x, y轴的数值, 基于屏幕中心点
                    mx += mdec.x;
                    my += mdec.y;
                    // 防止鼠标超出屏幕
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > bootinfo->screenx - 16) {
                        mx = bootinfo->screenx - 16;
                    }
                    if (my > bootinfo->screeny - 16) {
                        my = bootinfo->screeny - 16;
                    }
                    // 绘制
                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 0, COL8_FFFFFF, s); // 显示坐标
                    putblock8_8(bootinfo -> vram, bootinfo -> screenx, 16, 16, mx, my, mcursor, 16); // 显示鼠标
                }
            }
        }
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}
