#include "bootpack.h"
#include <stdio.h> // sprintf()

#define KEYCMD_LED      0xed

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0; // 获取asmhead.nas中存入的bootinfo信息
    char s[40];

    /* 键盘按键映射表 */
    static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',0,   '\\','Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '-', 0,  0,   0,   0,   0,   0,   0,   0,   0,   '\\', 0,  0
	};
    static char keytable1[0x80] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  0,   '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
    int currentwindow = 0, key_shift = 0, key_leds = (bootinfo->leds >> 4) & 7;

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
    // 初始化通用(键盘/鼠标/倒计时)缓冲区
    struct FIFO32 fifo;
    int fifobuf[128];
    fifo32_init(&fifo, 128, fifobuf, 0);

// 启用键盘/PIC1/鼠标(IRQ1, IRQ2, IRQ12)
	io_out8(PIC0_IMR, 0xf9); /* 开放键盘和PIC1中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
    init_keyboard(&fifo, 256); // 初始化键盘控制电路(包含鼠标控制电路)
    struct MOUSE_DEC mdec;
    enable_mouse(&fifo, 512, &mdec); // 启用鼠标本身

// 初始化键盘锁定键LED状态(CapsLock, NumLock, ScrollLock)
/*
    要控制键盘锁定键LED的状态, 需要向键盘发送EDXX数据
    其中XX的bit0代表ScrollLock, bit1代表NumLock, bit2代表CapsLock(0表示熄灭, 1表示点亮)
    这里先将EDXX放到缓冲区, 后续由缓冲区触发, 需先发送ED, 再发送XX, 共发送两次
*/
    struct FIFO32 keycmd;
    int keycmd_buf[32];
    int keycmd_wait = -1; // 用于控制是否需要发送数据到键盘(可以控制是否需要重发), -1代表可以发送下一数据
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

// 启用定时器(IRQ0)
    io_out8(PIC0_IMR, 0xf8); /* 开放定时器中断(11111000)*/
    struct TIMER *timer;
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50); // 0.5s

// 管理内存
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    unsigned int memorytotal;
    memorytotal = memtest(0x00400000, 0xbfffffff); // 获取总内存大小
    memmng_init(mng);
    memory_free(mng, 0x00001000, 0x0009e000); // 0x00001000~0x0009e000暂未使用, 释放掉
    memory_free(mng, 0x00400000, memorytotal - 0x00400000); // 0x00400000以后的内存也暂未使用, 释放掉

// 显示
    init_palette(); // 设定调色盘

// 管理图层
    struct LAYERCTL *layerctl;
    struct LAYER *layer_back, *layer_mouse, *layer_window, *layer_window_b[3], *layer_console;
    unsigned char *buf_back, buf_mouse[256], *buf_window, *buf_window_b, *buf_console;
    layerctl = layerctl_init(mng, bootinfo->vram, bootinfo->screenx, bootinfo->screeny); // 初始化图层管理
    // 背景图层
    layer_back = layer_alloc(layerctl); // 新建背景图层
    buf_back = (unsigned char *) memory_alloc_4k(mng, bootinfo->screenx * bootinfo->screeny); // 背景图层内容地址
    layer_init(layer_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 初始化背景图层
    init_screen8(buf_back, bootinfo -> screenx, bootinfo -> screeny); // 绘制背景
    // 绘制鼠标坐标(背景图层)
    int mx, my;
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
    int cursor_x, cursor_c;
    cursor_x = 8; // 光标位置
    cursor_c = COL8_FFFFFF; // 光标颜色

// 多任务
    // 任务控制器初始化, 并返回当前任务
    struct TASK *task_a, *task_b[3], *task_console;
    task_a = taskctl_init(mng);

    // 主任务
    task_run(task_a, 1, 0); // 更新task_a的层级信息为1
    fifo.task = task_a; // 若task_a休眠时有中断, 则唤醒task_a

    // 任务B(多窗口任务)
    int i;
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
        task_b[i]->tss.eip = (int) &task_b_implement;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8; // 使用段号2
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        // 往任务B传值
        task_b[i]->tss.esp -= 4; // 将要放入4字节参数, 压栈4字节
        *((int *) task_b[i]->tss.esp) = (int) layer_window_b[i]; // 将图层地址入栈
        task_b[i]->tss.esp -= 4; // 方法调用时返回地址保存在栈顶[ESP], 第一个参数保存在[ESP+4], 因此为了伪造方法调用, 压栈4字节
        // 启动任务B
        // task_run(task_b[i], 2, i + 1); // task层级为2, 优先级依次递增0.01
    }

    // 任务C(控制台任务)
    // 控制台图层
    layer_console = layer_alloc(layerctl);
    buf_console = (unsigned char *) memory_alloc_4k(mng, 256 * 165);
    layer_init(layer_console, buf_console, 256, 165, -1);
    make_window8(buf_console, 256, 165, "console", 0);
    make_textbox8(layer_console, 8, 28, 240, 128, COL8_000000);
    // 控制台任务
    task_console = task_alloc();
    // 任务TSS寄存器初始化
    task_console->tss.esp = memory_alloc_4k(mng, 64 * 1024) + 64 * 1024; // 任务使用的栈(64KB), esp存入栈顶(栈末尾高位地址)的地址
    task_console->tss.eip = (int) &console_task;
    task_console->tss.es = 1 * 8;
    task_console->tss.cs = 2 * 8; // 使用段号2
    task_console->tss.ss = 1 * 8;
    task_console->tss.ds = 1 * 8;
    task_console->tss.fs = 1 * 8;
    task_console->tss.gs = 1 * 8;
    // 往任务传值
    task_console->tss.esp -= 4; // 将要放入4字节参数, 压栈4字节
    *((int *) task_console->tss.esp) = (int) memorytotal; // 将内存信息入栈
    task_console->tss.esp -= 4; // 将要放入4字节参数, 压栈4字节
    *((int *) task_console->tss.esp) = (int) layer_console; // 将图层地址入栈
    task_console->tss.esp -= 4; // 方法调用时返回地址保存在栈顶[ESP], 第一个参数保存在[ESP+4], 因此为了伪造方法调用, 压栈4字节
    // 启动任务
    task_run(task_console, 2, 2); // task层级为2, 优先级为2

// 显示图层
    layer_slide(layer_back, 0, 0); // 移动背景图层
    layer_slide(layer_mouse, mx, my); // 移动鼠标图层到屏幕中点
    layer_slide(layer_window, 8, 56); // 移动窗口图层
    layer_slide(layer_window_b[0], 168, 56); // 移动窗口图层
    layer_slide(layer_window_b[1], 8, 116); // 移动窗口图层
    layer_slide(layer_window_b[2], 168, 116); // 移动窗口图层
    layer_slide(layer_console, 32, 176); // 移动窗口图层
    layer_updown(layer_back, 0); // 切换背景图层高度
    layer_updown(layer_mouse, 5); // 切换鼠标图层高度
    layer_updown(layer_window, 1); // 切换窗口图层高度
    layer_updown(layer_window_b[0], 1); // 切换窗口图层高度
    layer_updown(layer_window_b[1], 1); // 切换窗口图层高度
    layer_updown(layer_window_b[2], 1); // 切换窗口图层高度
    layer_updown(layer_console, 1); // 切换窗口图层高度

// 键盘和鼠标输入处理
    for (;;) {
        /* 键盘LEDS控制缓冲区处理 */
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            // 如果键盘缓冲区有数据, 则发送
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

        /* 中断缓冲区处理 */
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
                /* 显示键盘数据 */
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_layer(layer_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                /* 将按键编码转换为字符编码 */
                if (i < 256 + 0x80) {
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
                /* 大小写转换 */
                if ('A' <= s[0] && s[0] <= 'Z') {
                    // BOOTINFO->LEDS的bit6存储CapsLock(大小写锁定)状态(bit4:ScrollLock, bit5: NumLock), key_leds右移了4位, 因此现在存于bit2
                    if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
                        // CapsLock和Shift相同时, 转换为小写
                        s[0] += 0x20;
                    }
                }
                /* 不同字符编码处理 */
                if (s[0] != 0) {
                    /* 普通字符 */
                    if (currentwindow == 0) {
                        // 当前任务窗口, 直接显示
                        if (cursor_x < 128) {
                            s[1] = 0;
                            putfonts8_asc_layer(layer_window, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1); // 显示键盘按键
                            cursor_x += 8; // 光标前移
                        }
                    } else {
                        // 不同任务窗口, 发送到该任务绑定的中断缓冲区
                        fifo32_put(&task_console->fifo, s[0] + 256); // 发送对应键盘字符的ASCII
                    }
                }
                if (i == 256 + 0x0e) {
                    /* 退格键 */
                    if (currentwindow == 0) {
                        // 当前任务窗口, 直接显示
                        if (cursor_x > 8) {
                            putfonts8_asc_layer(layer_window, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1); // 擦除显示的键盘按键
                            cursor_x -= 8; // 光标后移
                        }
                    } else {
                        // 不同任务窗口, 发送到该任务绑定的中断缓冲区
                        fifo32_put(&task_console->fifo, 8 + 256); // 发送对应键盘字符的ASCII, ASCII中空格为8
                    }
                }
                if (i == 256 + 0x1c) {
                    /* 回车键 */
                    if (currentwindow != 0) {
                        fifo32_put(&task_console->fifo, 10 + 256); // 发送对应键盘字符的ASCII
                    }
                }
                if (i == 256 + 0x0f) {
                    /* TAB键 */
                    if (currentwindow == 0) {
                        currentwindow = 1;
                        // 标题栏切换
                        make_title8(buf_window, layer_window->bxsize, "task_a", 0);
                        make_title8(buf_console, layer_console->bxsize, "console", 1);
                        // 主窗口光标停止闪烁, 此处标志, 后续处理
                        cursor_c = -1;
                        // 主窗口隐藏光标(显示成背景色白色)
                        boxfill8(layer_window->buf, layer_window->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
                        // 目标窗口光标开始闪烁
                        fifo32_put(&task_console->fifo, 2);
                    } else {
                        currentwindow = 0;
                        // 标题栏切换
                        make_title8(buf_window, layer_window->bxsize, "task_a", 1);
                        make_title8(buf_console, layer_console->bxsize, "console", 0);
                        // 主窗口光标闪烁, 此处标志, 后续处理
                        cursor_c = COL8_000000;
                        // 目标窗口光标停止闪烁
                        fifo32_put(&task_console->fifo, 3);                        
                    }
                    layer_refresh(layer_window, 0 ,0, layer_window->bxsize, 21);
                    layer_refresh(layer_console, 0 ,0, layer_console->bxsize, 21);
                }
                /* Shitf键处理: 左Shift按下置1, 右Shift按下置2, 两个按下置3, 两个都不按置0 */
                if (i == 256 + 0x2a) {
                    /* 左Shift按下 */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {
                    /* 右Shift按下 */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) {
                    /* 左Shift抬起 */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) {
                    /* 右Shift抬起 */
                    key_shift &= ~2;
                }
                /* 各种锁定键处理key_leds(bit2: CapsLock, bit1: NumLock, bit0:ScrollLock) */
                if (i == 256 + 0x3a) {
                    /* CapsLock */
                    key_leds ^= 4;
                    // 向键盘发送EDXX, 控制LED状态
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) {
                    /* NUMLock */
                    key_leds ^= 2;
                    // 向键盘发送EDXX, 控制LED状态
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);                    
                }
                if (i == 256 + 0x46) {
                    /* ScrollLock */
                    key_leds ^= 1;
                    // 向键盘发送EDXX, 控制LED状态
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);                    
                }
                if (i == 256 + 0xfa) {
                    /* 键盘接收数据成功 */
                    keycmd_wait = -1; // 可以发送下一数据
                }
                if (i == 256 + 0xfe) {
                    /* 键盘接收数据失败, 再次发送 */
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }

                // 重绘光标
                // 是否显示光标
                if (cursor_c >= 0) {
                    boxfill8(layer_window->buf, layer_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43); // 显示白色
                }
                layer_refresh(layer_window, cursor_x, 28, cursor_x + 8, 44); // 刷新图层
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
            } else if (i <= 1) {
                // 光标闪烁定时器
                // 若为0则显示黑色, 若为1则显示白色, 交替进行, 实现闪烁效果
                if (i != 0)  {
                    timer_init(timer, &fifo, 0);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                } else {
                    timer_init(timer, &fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                }
                timer_settime(timer, 50); // 再次倒计时0.5s
                if (cursor_c >= 0) {
                    boxfill8(layer_window->buf, layer_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43); // 显示黑色
                    layer_refresh(layer_window, cursor_x, 28, cursor_x + 8, 44); // 刷新图层
                }
            }
        }
    }
}
