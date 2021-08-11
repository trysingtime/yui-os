#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char active);
void make_textbox8(struct LAYER *layer, int x0, int y0, int sx, int sy, int c);
void putfonts8_asc_layer(struct LAYER *layer, int x, int y, int color, int backcolor, char *string, int length);
void putfonts8_asc_layer(struct LAYER *layer, int x, int y, int color, int backcolor, char *string, int length);
void task_b_main(struct LAYER *layer_back);

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0; // 获取asmhead.nas中存入的bootinfo信息
    struct FIFO32 fifo;
    int fifobuf[128];
    char s[40];
    struct TIMER *timer;
    int mx, my, i, cursor_x, cursor_c;
    unsigned int memorytotal;
    struct MOUSE_DEC mdec;
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    struct LAYERCTL *layerctl;
    struct LAYER *layer_back, *layer_mouse, *layer_window, *layer_window_b[3];
    unsigned char *buf_back, buf_mouse[256], *buf_window, *buf_window_b;
    /* 键盘按键映射表 */
    static char keytable[0x54] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0,   '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};
    struct TASK *task_a, *task_b[3];

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
    fifo32_init(&fifo, 128, fifobuf, 0); // 初始化通用(键盘/鼠标/倒计时)缓冲区

// 启用键盘/PIC1/鼠标(IRQ1, IRQ2, IRQ12)
	io_out8(PIC0_IMR, 0xf9); /* 开放键盘和PIC1中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
    init_keyboard(&fifo, 256); // 初始化键盘控制电路(包含鼠标控制电路)
    enable_mouse(&fifo, 512, &mdec); // 启用鼠标本身

// 启用定时器(IRQ0)
    io_out8(PIC0_IMR, 0xf8); /* 开放定时器中断(11111000)*/
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50); // 0.5s

// 管理内存    
    memorytotal = memtest(0x00400000, 0xbfffffff); // 获取总内存大小
    memmng_init(mng);
    memory_free(mng, 0x00001000, 0x0009e000); // 0x00001000~0x0009e000暂未使用, 释放掉
    memory_free(mng, 0x00400000, memorytotal - 0x00400000); // 0x00400000以后的内存也暂未使用, 释放掉

// 显示
    init_palette(); // 设定调色盘

// 管理图层
    layerctl = layerctl_init(mng, bootinfo->vram, bootinfo->screenx, bootinfo->screeny); // 初始化图层管理
    // 背景图层
    layer_back = layer_alloc(layerctl); // 新建背景图层
    buf_back = (unsigned char *) memory_alloc_4k(mng, bootinfo->screenx * bootinfo->screeny); // 背景图层内容地址
    layer_init(layer_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 初始化背景图层
    init_screen8(buf_back, bootinfo -> screenx, bootinfo -> screeny); // 绘制背景
    // 绘制鼠标坐标(背景图层)
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_layer(layer_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    // 绘制内存信息(背景图层)
    sprintf(s, "memory %dMB     free : %dKB", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
    putfonts8_asc_layer(layer_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);
    // 绘制字符串(背景图层)
    putfonts8_asc_layer(layer_back, 100, 70, COL8_FFFFFF, COL8_008484, "Haribote OS.", 40);
    putfonts8_asc_layer(layer_back, 101, 71, COL8_000000, COL8_008484, "Haribote OS.", 40); // 文字阴影效果

    // 鼠标图层
    layer_mouse = layer_alloc(layerctl); // 新建鼠标图层
    layer_init(layer_mouse, buf_mouse, 16, 16, 99); // 初始化鼠标图层(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)
    init_mouse_cursor8(buf_mouse, 99); // 绘制鼠标指针(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)

    // 窗口图层
    layer_window = layer_alloc(layerctl); // 新建窗口图层
    buf_window = (unsigned char *) memory_alloc_4k(mng, 160 * 52);
    layer_init(layer_window, buf_window, 144, 52, -1); // 初始化窗口图层
    make_window8(buf_window, 144, 52, "task_a", 1); // 绘制窗口
    // 绘制窗口图层-文本框
    make_textbox8(layer_window, 8, 28, 144, 16, COL8_FFFFFF);
    cursor_x = 8; // 光标位置
    cursor_c = COL8_FFFFFF; // 光标颜色

// 多任务
    // 任务控制器初始化, 并返回当前任务
    task_a = taskctl_init(mng);
    task_run(task_a, 1, 0); // 更新task_a的层级信息为1
    fifo.task = task_a; // 若task_a休眠时有中断, 则唤醒task_a

    // 多窗口和多任务
    for (i = 0; i < 3; i++) {
        // 窗口图层
        layer_window_b[i] = layer_alloc(layerctl); // 新建窗口图层
        buf_window_b = (unsigned char *) memory_alloc_4k(mng, 144 * 52);
        layer_init(layer_window_b[i], buf_window_b, 144, 52, -1); // 初始化窗口图层
        sprintf(s, "task_b%d", i);
        make_window8(buf_window_b, 144, 52, s, 0); // 绘制窗口

        // 任务B初始化
        task_b[i] = task_alloc();
        // 任务B, TSS寄存器初始化
        task_b[i]->tss.esp = memory_alloc_4k(mng, 64 * 1024) + 64 * 1024; // 任务B使用的栈(64KB), esp存入栈顶(栈末尾高位地址)的地址
        task_b[i]->tss.eip = (int) &task_b_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8; // 使用段号2
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        // 往任务B传值
        task_b[i]->tss.esp -= 4; // 将要放入4字节参数, 压栈4字节
        *((int *) task_b[i]->tss.esp) = (int) layer_window_b[i]; // 将内存地址入栈
        task_b[i]->tss.esp -= 4; // 方法调用时返回地址保存在栈顶[ESP], 第一个参数保存在[ESP+4], 因此为了伪造方法调用, 压栈4字节
        // 启动任务B
        task_run(task_b[i], 2, i + 1); // task层级为2, 优先级依次递增0.01
    }

// 显示图层
    layer_slide(layer_back, 0, 0); // 移动背景图层
    layer_slide(layer_mouse, mx, my); // 移动鼠标图层到屏幕中点
    layer_slide(layer_window, 8, 56); // 移动窗口图层
    layer_slide(layer_window_b[0], 168, 56); // 移动窗口图层
    layer_slide(layer_window_b[1], 8, 116); // 移动窗口图层
    layer_slide(layer_window_b[2], 168, 116); // 移动窗口图层
    layer_updown(layer_back, 0); // 切换背景图层高度
    layer_updown(layer_mouse, 5); // 切换鼠标图层高度
    layer_updown(layer_window, 4); // 切换窗口图层高度
    layer_updown(layer_window_b[0], 1); // 切换窗口图层高度
    layer_updown(layer_window_b[1], 2); // 切换窗口图层高度
    layer_updown(layer_window_b[2], 3); // 切换窗口图层高度

// 键盘和鼠标输入处理
    for (;;) {
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            // 缓冲区为空
            task_sleep(task_a); // 若没有中断, 则task_a休眠, 休眠后再开启中断, 防止无法休眠
            // io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
            io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 缓冲区存在信息
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {
                // 键盘缓冲区处理
                // 显示键盘数据
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_layer(layer_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 256 + 0x54) {
                    // 键盘数据含于键盘按键表中
                    if (keytable[i - 256] != 0 && cursor_x < 144) {
                        /* 普通字符 */
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfonts8_asc_layer(layer_window, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1); // 显示键盘按键
                        cursor_x += 8; // 光标前移
                    }
                    if (i == 256 + 0x0e && cursor_x > 8) {
                        /* 退格键 */
                        putfonts8_asc_layer(layer_window, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1); // 擦除显示的键盘按键
                        cursor_x -= 8; // 光标后移
                    }
                    // 重绘光标
                    boxfill8(layer_window->buf, layer_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43); // 显示白色
                    layer_refresh(layer_window, cursor_x, 28, cursor_x + 8, 44); // 刷新图层
                }
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
                    mx += mdec.x/3;
                    my += mdec.y/3;
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
                    // 鼠标左键
                    if ((mdec.btn & 0x01) != 0) {
                        // 移动窗口
                        layer_slide(layer_window, mx - 80, my - 8);
                    }
                }
            } else if (i == 1)  {
                // 光标闪烁定时器
                // 若为0则显示黑色, 若为1则显示白色, 交替进行, 实现闪烁效果
                timer_init(timer, &fifo, 0);
                timer_settime(timer, 50); // 再次倒计时0.5s
                cursor_c = COL8_000000;
                boxfill8(layer_window->buf, layer_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43); // 显示白色
                layer_refresh(layer_window, cursor_x, 28, cursor_x + 8, 44); // 刷新图层
            } else if (i == 0) {
                // 光标闪烁定时器
                // 若为0则显示黑色, 若为1则显示白色, 交替进行, 实现闪烁效果
                timer_init(timer, &fifo, 1);
                timer_settime(timer, 50); // 再次倒计时0.5s
                cursor_c = COL8_FFFFFF;
                boxfill8(layer_window->buf, layer_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43); // 显示黑色
                layer_refresh(layer_window, cursor_x, 28, cursor_x + 8, 44); // 刷新图层
            }
        }
    }
}

/*
    绘制窗体
    buf: 窗体内容起始地址
    xsize, ysize: 窗体大小
    titile: 窗体标题
    activa: 该窗口是否激活
*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char active) {
    /* 若窗口激活则标题栏显示暗青, 否则显示暗灰 */
    char title_color, title_backgroud_color;
    if (active != 0) {
        title_color = COL8_FFFFFF;
        title_backgroud_color = COL8_000084;
    } else {
        title_color = COL8_C6C6C6;
        title_backgroud_color = COL8_848484;
    }
	boxfill8(buf, xsize, COL8_C6C6C6,           0,         0,         xsize - 1, 0        ); // 上边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF,           1,         1,         xsize - 2, 1        ); // 上边界-内阴影-白
	boxfill8(buf, xsize, COL8_C6C6C6,           0,         0,         0,         ysize - 1); // 左边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF,           1,         1,         1,         ysize - 2); // 左边界-内阴影-白
	boxfill8(buf, xsize, COL8_848484,           xsize - 2, 1,         xsize - 2, ysize - 2); // 右边界-暗灰
	boxfill8(buf, xsize, COL8_000000,           xsize - 1, 0,         xsize - 1, ysize - 1); // 右边界-阴影-黑
	boxfill8(buf, xsize, COL8_C6C6C6,           2,         2,         xsize - 3, ysize - 3); // 窗体-亮灰
	boxfill8(buf, xsize, title_backgroud_color, 3,         3,         xsize - 4, 20       ); // 标题栏-暗青/暗灰
	boxfill8(buf, xsize, COL8_848484,           1,         ysize - 2, xsize - 2, ysize - 2); // 下边界-暗灰
	boxfill8(buf, xsize, COL8_000000,           0,         ysize - 1, xsize - 1, ysize - 1); // 下边界-阴影-黑
	putfonts8_asc(buf, xsize, 24, 4, title_color, title); // 标题

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
    绘制文本框
    - layer: 指定图层
    - x0, y0: 文本框在图层中的坐标(左边和上边溢出3个像素, 右边和下边溢出2个像素)
    - xsize, ysize: 文本框长度和宽度
    - color: 文本框背景色
*/
void make_textbox8(struct LAYER *layer, int x0, int y0, int xsize, int ysize, int color) {
    int x1 = x0 + xsize, y1 = y0 + ysize;
    boxfill8(layer->buf, layer->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3); // 上边界-暗灰
    boxfill8(layer->buf, layer->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2); // 上边界-内阴影-黑
    boxfill8(layer->buf, layer->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1); // 左边界-亮灰
    boxfill8(layer->buf, layer->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0); // 左边界-内阴影-黑
    boxfill8(layer->buf, layer->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2); // 下边界-白
    boxfill8(layer->buf, layer->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1); // 下边界-内阴影-亮灰
    boxfill8(layer->buf, layer->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2); // 右边界-白
    boxfill8(layer->buf, layer->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1); // 右边界-内阴影-亮灰
    boxfill8(layer->buf, layer->bxsize, color      , x0 - 1, y0 - 1, x1 + 0, y1 + 0); // 文本输入框
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

/*
    任务B
*/
void task_b_main(struct LAYER *layer) {
    struct FIFO32 fifo;
    struct TIMER *timer_ls;
    int i, fifobuf[128], count = 0, count0 = 0;
    char s[12];

    // 设置中断缓冲区
    fifo32_init(&fifo, 128, fifobuf, 0);
    // 设置定时器
    timer_ls = timer_alloc(); // 运行速度测试定时器
    timer_init(timer_ls, &fifo, 100);
    timer_settime(timer_ls, 100); // 1s

    // 处理中断
    for (;;) {
        // 计数
        count++;
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            //io_stihlt();
            io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 定时器中断处理
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 100) {
                sprintf(s, "%11d", count - count0);
                putfonts8_asc_layer(layer, 24, 28, COL8_000000, COL8_C6C6C6, s, 11);
                count0 = count;
                timer_settime(timer_ls, 100); // 重置计时器, 再次计时
            }
        }
    }
}
