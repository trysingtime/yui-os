#include "bootpack.h"
#include <stdio.h>

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my, i;
    struct MOUSE_DEC mdec;
    unsigned int memorytotal;
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    struct LAYERCTL *layerctl;
    struct LAYER *layer_back, *layer_mouse;
    unsigned char *buf_back, buf_mouse[256];

// 设置系统参数
    init_gdtidt(); // 初始化GDT/IDT
    init_pic(); // 初始化PIC
    io_sti(); // 允许中断

// 启用键盘鼠标
    fifo8_init(&keyfifo, 32, keybuf); // 初始化键盘缓冲区
    fifo8_init(&mousefifo, 128, mousebuf); // 初始化鼠标缓冲区
    // 开放PIC, 键盘是IRQ1, PIC1是IRQ2, 鼠标是IRQ12
	io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

    init_keyboard(); // 初始化键盘控制电路(包含鼠标控制电路)
    enable_mouse(&mdec); // 启用鼠标本身

// 管理内存    
    memorytotal = memtest(0x00400000, 0xbfffffff); // 获取总内存大小
    memmng_init(mng);
    memory_free(mng, 0x00001000, 0x0009e000); // 0x00001000~0x0009e000暂未使用, 释放掉
    memory_free(mng, 0x00400000, memorytotal - 0x00400000); // 0x00400000以后的内存也暂未使用, 释放掉

// 管理图层
    layerctl = layerctl_init(mng, bootinfo->vram, bootinfo->screenx, bootinfo->screeny); // 初始化图层管理
    layer_back = layer_alloc(layerctl); // 新建背景图层
    layer_mouse = layer_alloc(layerctl); // 新建鼠标图层
    buf_back = (unsigned char *) memory_alloc_4k(mng, bootinfo->screenx * bootinfo->screeny); // 背景图层内容地址
    layer_init(layer_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 初始化背景图层
    layer_init(layer_mouse, buf_mouse, 16, 16, 99); // 初始化鼠标图层(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)

// 显示
    init_palette(); // 设定调色盘
    init_screen8(buf_back, bootinfo -> screenx, bootinfo -> screeny); // 绘制背景
    init_mouse_cursor8(buf_mouse, 99); // 绘制鼠标指针(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)

    layer_slide(layer_mouse, 0, 0); // 移动鼠标图层到(0, 0)并刷新所有图层
    layer_updown(layer_back, 0); // 切换背景图层高度并刷新所有图层
    layer_updown(layer_mouse, 1); // 切换鼠标图层高度并刷新所有图层
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)
    layer_slide(layer_mouse, mx, my); // 移动鼠标图层到屏幕中点

    // 绘制鼠标坐标(背景图层)
    sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(buf_back, bootinfo->screenx, 0, 0, COL8_FFFFFF, s); // 绘制鼠标坐标到背景图层
    // 绘制内存信息(背景图层)
    sprintf(s, "memory %dMB     free : %dKB", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
	putfonts8_asc(buf_back, bootinfo->screenx, 0, 32, COL8_FFFFFF, s);
    // 刷新图层
    layer_refresh(layer_back, 0, 0, bootinfo->screenx, 48);
    // 绘制字符串(背景图层)
 	// putfonts8_asc(bootinfo->vram, bootinfo->screenx,  8,  8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc(buf_back, bootinfo->screenx, 101, 71, COL8_000000, "Haribote OS."); // 文字阴影效果
	putfonts8_asc(buf_back, bootinfo->screenx, 100, 70, COL8_FFFFFF, "Haribote OS.");
    // 刷新图层
    layer_refresh(layer_back, 100, 70, 71 + 15 * 8 - 1, 86);

// 键盘和鼠标输入处理
    for (;;) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
        } else {
            if (fifo8_status(&keyfifo) != 0 ) {
                i = fifo8_get(&keyfifo);
                io_sti();

                // 显示键盘数据
                boxfill8(buf_back, bootinfo->screenx, COL8_008484, 0, 16, 15, 31); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
                sprintf(s, "%02X", i);
                putfonts8_asc(buf_back, bootinfo->screenx, 0, 16, COL8_FFFFFF, s); // 显示新的数据
                layer_refresh(layer_back, 0, 16, 15, 31);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();

                if (mouse_decode(&mdec, i) == 1) {
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
                    // 显示鼠标数据
                    boxfill8(buf_back, bootinfo->screenx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
                    putfonts8_asc(buf_back, bootinfo->screenx, 32, 16, COL8_FFFFFF, s); // 显示新的数据
                    layer_refresh(layer_back, 32, 16, 32 + 15 * 8 - 1, 31);

                    // 显示鼠标指针移动
                    // 计算鼠标x, y轴的数值, 基于屏幕中心点
                    mx += mdec.x / 10;
                    my += mdec.y / 10;
                    // 防止鼠标超出屏幕
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    // 鼠标可以超出右边和底边, 直到保留一个像素
                    if (mx > bootinfo->screenx - 1) {
                        mx = bootinfo->screenx - 1;
                    }
                    if (my > bootinfo->screeny - 1) {
                        my = bootinfo->screeny - 1;
                    }
                    // 显示鼠标坐标数据
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, bootinfo -> screenx, COL8_008484, 0, 0, 79, 15); // 擦除原有坐标(绘制背景色矩形遮住之前绘制好的坐标)
                    putfonts8_asc(buf_back, bootinfo->screenx, 0, 0, COL8_FFFFFF, s); // 显示新的坐标
                    layer_refresh(layer_back, 0, 0, 79, 15);
                    // 移动鼠标
                    layer_slide(layer_mouse, mx, my); // 显示鼠标
                }
            }
        }
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}
