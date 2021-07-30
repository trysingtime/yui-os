#include "bootpack.h"
#include <stdio.h>

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0; // 获取asmhead.nas中存入的bootinfo信息
    char s[40], keybuf[32], mousebuf[128];
    int mx, my, i;
    unsigned int memorytotal, count = 0;
    struct MOUSE_DEC mdec;
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    struct LAYERCTL *layerctl;
    struct LAYER *layer_back, *layer_mouse, *layer_window;
    unsigned char *buf_back, *buf_window, buf_mouse[256];

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
    layer_window = layer_alloc(layerctl); // 新建窗口图层
    layer_mouse = layer_alloc(layerctl); // 新建鼠标图层
    buf_back = (unsigned char *) memory_alloc_4k(mng, bootinfo->screenx * bootinfo->screeny); // 背景图层内容地址
    buf_window = (unsigned char *) memory_alloc_4k(mng, 160 * 52);
    layer_init(layer_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 初始化背景图层
    layer_init(layer_mouse, buf_mouse, 16, 16, 99); // 初始化鼠标图层(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)
    layer_init(layer_window, buf_window, 160, 52, -1); // 初始化窗口图层

// 显示
    init_palette(); // 设定调色盘
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)

    // 绘制背景
    init_screen8(buf_back, bootinfo -> screenx, bootinfo -> screeny);
    // 绘制窗口
    make_window8(buf_window, 160, 52, "counter");
    // 绘制鼠标指针(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)
    init_mouse_cursor8(buf_mouse, 99); 

    // 显示图层
    layer_slide(layer_back, 0, 0); // 移动背景图层
    layer_slide(layer_window, 80, 72); // 移动窗口图层
    layer_slide(layer_mouse, mx, my); // 移动鼠标图层到屏幕中点
    layer_updown(layer_back, 0); // 切换背景图层高度
    layer_updown(layer_window, 1); // 切换窗口图层高度
    layer_updown(layer_mouse, 2); // 切换鼠标图层高度

    // 绘制鼠标坐标(背景图层)
    sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(buf_back, bootinfo->screenx, 0, 0, COL8_FFFFFF, s); // 绘制鼠标坐标到背景图层
    // 绘制内存信息(背景图层)
    sprintf(s, "memory %dMB     free : %dKB", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
	putfonts8_asc(buf_back, bootinfo->screenx, 0, 32, COL8_FFFFFF, s);
    // 刷新图层
    layer_refresh(layer_back, 0, 0, bootinfo->screenx, 48);
    // 绘制字符串(背景图层)
	putfonts8_asc(buf_back, bootinfo->screenx, 101, 71, COL8_000000, "Haribote OS."); // 文字阴影效果
	putfonts8_asc(buf_back, bootinfo->screenx, 100, 70, COL8_FFFFFF, "Haribote OS.");
    // 刷新图层
    layer_refresh(layer_back, 100, 70, 71 + 15 * 8 - 1, 86);

// 键盘和鼠标输入处理
    for (;;) {
        // 显示计数器窗口
        count++;
        sprintf(s, "%010d", count);
        boxfill8(buf_window, 160, COL8_C6C6C6, 40, 28, 119, 43); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
        putfonts8_asc(buf_window, 160, 40, 28, COL8_000000, s); // 显示新的数据
        layer_refresh(layer_window, 40, 28, 120, 44); // 刷新图层

        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            // io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
            io_sti(); // 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            if (fifo8_status(&keyfifo) != 0 ) {
                i = fifo8_get(&keyfifo);
                io_sti();

                // 显示键盘数据
                sprintf(s, "%02X", i);
                boxfill8(buf_back, bootinfo->screenx, COL8_008484, 0, 16, 15, 31); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
                putfonts8_asc(buf_back, bootinfo->screenx, 0, 16, COL8_FFFFFF, s); // 显示新的数据
                layer_refresh(layer_back, 0, 16, 15, 31); // 刷新图层
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
                    layer_refresh(layer_back, 32, 16, 32 + 15 * 8 - 1, 31); // 刷新图层

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
                    layer_refresh(layer_back, 0, 0, 79, 15); // 刷新图层
                    // 移动鼠标
                    layer_slide(layer_mouse, mx, my); // 显示鼠标
                }
            }
        }
    }
}

/*
    绘制窗体
    buf: 窗体内容起始地址
    xsize, ysize: 窗体大小
    titile: 窗体标题
*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        ); // 上边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        ); // 上边界阴影-白
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1); // 左边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2); // 左边界阴影-白
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2); // 右边界-暗灰
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1); // 右边界阴影-黑
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3); // 窗体-亮灰
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       ); // 标题栏-暗青
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2); // 下边界-暗灰
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1); // 下边界阴影-黑
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title); // 标题

    // 右上角按钮X, 坐标(5, xsize - 21), 大小(14 * 16)
    static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}
