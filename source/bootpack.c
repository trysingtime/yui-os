#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void putfonts8_asc_layer(struct LAYER *layer, int x, int y, int color, int backcolor, char *string, int length);
void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0; // 获取asmhead.nas中存入的bootinfo信息
    struct FIFO32 fifo;
    int fifobuf[128];
    char s[40];
    struct TIMER *timer, *timer2, *timer3;
    int mx, my, i, count = 0;
    unsigned int memorytotal;
    struct MOUSE_DEC mdec;
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    struct LAYERCTL *layerctl;
    struct LAYER *layer_back, *layer_mouse, *layer_window;
    unsigned char *buf_back, *buf_window, buf_mouse[256];

// 设置系统参数
    init_gdtidt(); // 初始化GDT/IDT
    init_pic(); // 初始化PIC
    init_pit(); // 初始化PIT
    io_sti(); // 允许中断

// 设置中断缓冲区
/*
    FIFO值      中断类型
    0~1         光标闪烁定时器
    3           3秒定时器
    10          10秒定时器
    256~511     键盘输入(键盘控制器读入的值再加上256)
    512~767     鼠标输入(键盘控制器读入的值再加上512)
*/
    fifo32_init(&fifo, 128, fifobuf); // 初始化通用(键盘/鼠标/倒计时)缓冲区

// 启用键盘/PIC1/鼠标(IRQ1, IRQ2, IRQ12)
	io_out8(PIC0_IMR, 0xf9); /* 开放键盘和PIC1中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
    init_keyboard(&fifo, 256); // 初始化键盘控制电路(包含鼠标控制电路)
    enable_mouse(&fifo, 512, &mdec); // 启用鼠标本身

// 启用定时器(IRQ0)
    io_out8(PIC0_IMR, 0xf8); /* 开放定时器中断(11111000)*/
    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000); //10s
    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300); //3s
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50); // 0.5s

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
    putfonts8_asc_layer(layer_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    // 绘制内存信息(背景图层)
    sprintf(s, "memory %dMB     free : %dKB", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
    putfonts8_asc_layer(layer_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);
    // 绘制字符串(背景图层)
    putfonts8_asc_layer(layer_back, 100, 70, COL8_FFFFFF, COL8_008484, "Haribote OS.", 40);
    putfonts8_asc_layer(layer_back, 101, 71, COL8_000000, COL8_008484, "Haribote OS.", 40); // 文字阴影效果

// 键盘和鼠标输入处理
    for (;;) {
        // 用于性能测试, 排除开机前3秒, 从第3秒测试到10秒
        count++;

        io_cli();
        if (fifo32_status(&fifo) == 0) {
            // 缓冲区为空
            // io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
            io_sti(); // 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 缓冲区存在信息
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {
                // 键盘缓冲区处理
                // 显示键盘数据
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_layer(layer_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
            } else if (512 <= i && i <= 767) {
                // 鼠标缓冲区处理
                if (mouse_decode(&mdec, i - 512) == 1) {
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
                    putfonts8_asc_layer(layer_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

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
                    putfonts8_asc_layer(layer_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    // 移动鼠标
                    layer_slide(layer_mouse, mx, my); // 显示鼠标
                }
            } else if (i == 10) {
                // 10s定时器, 显示字符串
                putfonts8_asc_layer(layer_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7); // 显示10[sec]
                // 性能测试, 排除开机前3秒, 从第3秒测试到10秒
                sprintf(s, "%010d", count);
                putfonts8_asc_layer(layer_window, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
            } else if (i == 3) {
                // 3s定时器, 显示字符串
                putfonts8_asc_layer(layer_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6); // 显示3[sec]
                // 性能测试, 排除开机前3秒, 从第3秒测试到10秒
                count = 0;
            } else if (i == 1)  {
                // 光标闪烁定时器
                // 若为0则显示背景色, 若为1则显示白色, 交替进行, 实现闪烁效果
                timer_init(timer3, &fifo, 0);
                boxfill8(buf_back, bootinfo->screenx, COL8_FFFFFF, 8, 96, 15, 111); // 显示白色
                timer_settime(timer3, 50); // 再次倒计时0.5s
                layer_refresh(layer_back, 8, 96, 16, 112); // 刷新图层
            } else if (i == 0) {
                // 光标闪烁定时器
                // 若为0则显示背景色, 若为1则显示白色, 交替进行, 实现闪烁效果
                timer_init(timer3, &fifo, 1);
                boxfill8(buf_back, bootinfo->screenx, COL8_008484, 8, 96, 15, 111); // 显示背景色
                timer_settime(timer3, 50); // 再次倒计时0.5s
                layer_refresh(layer_back, 8, 96, 16, 112); // 刷新图层
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

/*
    在指定图层绘制字符串
    - layer: 指定图层
    - x,y: 指定图层内部坐标
    - color: 字符串颜色
    - backcolor: 背景颜色
    - string: 字符串地址
    - length: 字符串长度
*/
void putfonts8_asc_layer(struct LAYER *layer, int x, int y, int color, int backcolor, char *string, int length) {
        boxfill8(layer->buf, layer->bxsize, backcolor, x, y, x + length * 8 - 1, y + 15); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
        putfonts8_asc(layer->buf, layer->bxsize, x, y, color, string); // 显示新的数据
        layer_refresh(layer, x, y, x + length * 8, y + 16); // 刷新图层
        return;
}
