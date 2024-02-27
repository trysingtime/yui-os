#include "bootpack.h"
#include <stdio.h> // sprintf()

#define KEYCMD_LED      0xed
struct LAYER *keyboard_input_layer; // 键盘输入的窗口图层
struct LAYER *mouse_click_layer; // 鼠标输入的窗口图层

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0; // 获取asmhead.nas中存入的bootinfo信息
    char s[40];

    /* 键盘按键映射表 */
    static char keytable0[0x80] = {
		0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x0a,  0,  'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',0,   '\\','Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '-', 0,  0,   0,   0,   0,   0,   0,   0,   0,   '\\', 0,  0
	};
    static char keytable1[0x80] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08,  0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a,  0,  'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  0,   '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
    int key_shift = 0, key_leds = (bootinfo->leds >> 4) & 7;

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
    *((int *) 0x0fec) = (int) &fifo; // 将通用缓冲区地址放入0xfec, 便于app调用系统api    

// 启用键盘/PIC1/鼠标(IRQ1, IRQ2, IRQ12)
	io_out8(PIC0_IMR, 0xf9); /* 开放键盘和PIC1中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
    init_keyboard(&fifo, 256); // 初始化键盘控制电路(包含鼠标控制电路)
    struct MOUSE_DEC mdec;
    enable_mouse(&fifo, 512, &mdec); // 启用鼠标本身
    int mx = (bootinfo -> screenx - 16) / 2; // 鼠标坐标x: 计算屏幕中间点(减去指针本身)
    int my = (bootinfo -> screeny - 28 - 16) / 2; // 鼠标坐标y: 计算屏幕中间点(减去任务栏和指针本身)
    int new_mouse_x = -1, new_mouse_y = -1; // 鼠标移动后,图层刷新前的新坐标

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
    // timer_settime(timer, 50); // 0.5s

// 管理内存
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    unsigned int memorytotal;
    memorytotal = memtest(0x00400000, 0xbfffffff); // 获取总内存大小
    memmng_init(mng);
    memory_free(mng, 0x00001000, 0x0009e000); // 0x00001000~0x0009e000暂未使用, 释放掉
    memory_free(mng, 0x00400000, memorytotal - 0x00400000); // 0x00400000以后的内存也暂未使用, 释放掉

// 载入字体
    int i;
    // 读取FAT
    int *fat = (int *)memory_alloc_4k(mng, 4 * 2880);
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x0200));

// 载入中文字体(不包含半角符号)
    unsigned char *chinese;
    // 根据文件名查找
    struct FILEINFO *finfo = file_search("chinese.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x2600), 224);
    if (finfo != 0) {
        /* 存在字库文件 */
        // 根据FAT读取文件内容
        i = finfo->size; // 使用中间变量, 防止finfo-size被直接修改
        chinese = file_load_compressfile(finfo->clustno, &i, fat);
    } else {
        /* 没有字库文件 */
        chinese = (unsigned char *) memory_alloc_4k(mng, 32 * 94 * 55); // 中文分为94区(此处仅需要0~55区), 每区包含94个字体, 每个字体32字节
        for (i = 0; i < 32 * 94 * 55; i++) {
            chinese[i] = 0xff;
        }
    }
    // 将中文字体地址放入0x0fe0
    *((int *) 0x0fe8) = (int) chinese;
// 载入日语字体(包含半角符号)
    unsigned char *nihongo;
    // 根据文件名查找
    finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x2600), 224);
    if (finfo != 0) {
        /* 存在字库文件 */
        // 根据FAT读取文件内容
        i = finfo->size; // 使用中间变量, 防止finfo-size被直接修改
        nihongo = file_load_compressfile(finfo->clustno, &i, fat);
    } else {
        /* 没有字库文件 */
        nihongo = (unsigned char *) memory_alloc_4k(mng, 16 * 256 + 32 * 94 * 47); // 日文分为94区(此处仅需要0~47区), 每区包含94个字体, 每个字体32字节, 且前256字符为半角符号
        // 前256字符使用系统内置字库
        extern char hankaku[4096];
        for (i = 0; i < 16 * 256; i++) {
            nihongo[i] = hankaku[i];
        }
        // 后续字符使用0xff覆盖
        for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
            nihongo[i] = 0xff;
        }
    }
    // 将日语字体地址放入0x0fe0
    *((int *) 0x0fe0) = (int) nihongo;
    // 释放FAT内存空间
    memory_free_4k(mng, (int) fat, 4 * 2880);

// 显示
    init_palette(); // 设定调色盘

// 多任务
    // 任务控制器初始化, 并返回当前任务
    struct TASK *task_a;
    task_a = taskctl_init(mng);
// 主任务task_a
    task_run(task_a, 1, 0); // 更新task_a的层级信息为1
    // 主任务语言模式
    task_a->langmode = 0; // 默认英文模式
    task_a->langbuf = 0;
    // 主任务缓冲区
    int task_a_fifo[128];
    fifo32_init(&task_a->fifo, 128, task_a_fifo, 0); // 无自动唤醒
    fifo.task = task_a; // 通用缓冲区fifo绑定任务(若task_a休眠时有中断输入fifo, 则唤醒task_a)
    // 主任务图层
    struct LAYERCTL *layerctl;
    layerctl = layerctl_init(mng, bootinfo->vram, bootinfo->screenx, bootinfo->screeny); // 初始化图层管理
    *((int *) 0x0fe4) = (int) layerctl; // 将图层管理器地址放入0x0fe4, 便于app调用系统api
    // 背景图层
    struct LAYER *layer_back;
    unsigned char *buf_back;
    layer_back = layer_alloc(layerctl); // 新建背景图层
    buf_back = (unsigned char *) memory_alloc_4k(mng, bootinfo->screenx * bootinfo->screeny); // 背景图层内容地址
    layer_init(layer_back, buf_back, bootinfo->screenx, bootinfo->screeny, -1); // 初始化背景图层
    init_screen8(buf_back, bootinfo -> screenx, bootinfo -> screeny); // 绘制背景
    // 绘制鼠标坐标(背景图层)
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_layer(layer_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    // 绘制内存信息(背景图层)
    sprintf(s, "memory %dMB     free : %dKB", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
    putfonts8_asc_layer(layer_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);
    // 绘制字符串(背景图层)
    putfonts8_asc_layer(layer_back, 100, 70, COL8_FFFFFF, COL8_008484, "Haribote OS.", 40);
    putfonts8_asc_layer(layer_back, 101, 71, COL8_000000, COL8_008484, "Haribote OS.", 40); // 文字阴影效果
    // 鼠标图层
    struct LAYER *layer_mouse;
    unsigned char buf_mouse[256];
    layer_mouse = layer_alloc(layerctl); // 新建鼠标图层
    layer_init(layer_mouse, buf_mouse, 16, 16, 99); // 初始化鼠标图层(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)
    init_mouse_cursor8(buf_mouse, 99); // 绘制鼠标指针(图层颜色设置为未使用的99, 鼠标背景颜色也设置为99, 两者相同则透明)

// 控制台任务
    // 控制台图层及任务
    struct LAYER *layer_console = open_console(layerctl, memorytotal);

// 显示图层
    layer_slide(layer_back, 0, 0); // 移动背景图层
    layer_slide(layer_mouse, mx, my); // 移动鼠标图层到屏幕中点
    layer_slide(layer_console, 100, 100); // 移动控制台图层
    // 实际图层高度由已有图层决定, 而不是指定值, 相同高度, 后者更低
    layer_updown(layer_back,        0); // 切换背景图层高度
    layer_updown(layer_console,     1); // 切换控制台图层高度
    layer_updown(layer_mouse,       2); // 切换鼠标图层高度

// 图形化界面控制
    mouse_click_layer = 0; // 鼠标输入的窗口图层
    keyboard_input_layer = layer_console; // 键盘输入的窗口图层
    // 窗口控制
    switch_window(layerctl, 0, keyboard_input_layer);
    int old_mouse_x = -1, old_mouse_y = -1; // 鼠标拖动窗口前, 鼠标的坐标
    int old_window_x = 0, old_window_y = 0; // 鼠标拖动窗口前, 窗口的坐标
    int new_window_x = 0x7fffffff, new_window_y = 0; // 鼠标拖动窗口后, 窗口的坐标

// 键盘和鼠标输入处理
    for (;;) {
        /* 键盘LEDS控制缓冲区处理 */
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            // 如果键盘缓冲区有数据, 则发送
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

        /* task_a缓冲区 */
        if(fifo32_status(&task_a->fifo) > 0) {
            // 将数据转发到通用缓冲区
            i = fifo32_get(&task_a->fifo);
            fifo32_put(&fifo, i);
        }

        /* 中断缓冲区处理 */
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            // 缓冲区为空
            if (new_mouse_x >= 0) {
                /* 绘制鼠标移动 */
                io_sti();
                layer_slide(layer_mouse, new_mouse_x, new_mouse_y);
                new_mouse_x = -1;
            } else if (new_window_x != 0x7fffffff) {
                /* 绘制窗口拖动 */
                io_sti();
                layer_slide(mouse_click_layer, new_window_x, new_window_y);
                new_window_x = 0x7fffffff;
            } else {
                /* 休眠 */
                task_sleep(task_a); // 若没有中断, 则task_a休眠, 休眠后再开启中断, 防止无法休眠
                // io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
                io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
            }
        } else {
            // 缓冲区存在信息
            i = fifo32_get(&fifo);
            io_sti();
            // 若当前键盘输入窗口被关闭, 则切换键盘输入的窗口
            if (keyboard_input_layer != 0 && keyboard_input_layer->flags == 0) {
                if (layerctl->top == 1) {
                    /* 所有图层被关闭, 只剩背景图层和鼠标图层 */
                    keyboard_input_layer = 0;
                } else {
                    /* 切换到下一图层 */
                    switch_window(layerctl, keyboard_input_layer, 0);
                }
            }
            // 输入处理
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
                if (s[0] != 0 && keyboard_input_layer != 0) {
                    /* 普通字符/退格键/回车键: 发送键盘字符到该任务绑定的中断缓冲区 */
                    fifo32_put(&keyboard_input_layer->task->fifo, s[0] + 256); // 发送对应键盘字符的ASCII
                }
                if (i == 256 + 0x0f) {
                    /* TAB键: 切换窗口图层 */
                    /* 存在2个以上的图层(包括背景图层和鼠标图层)才能切换 */
                    if (layerctl->top > 2) {
                        switch_window(layerctl, keyboard_input_layer, 0);
                    }
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
                if (i == 256 + 0x57) {
                    /* F11按键: 切换窗口 */
                    /* 存在2个以上的图层(包括背景图层和鼠标图层)才能切换 */
                    if (layerctl->top > 2) {
                        // 倒数第二层提升到正数第二层(最底层是背景图层, 最高层是鼠标图层, 都不用变动)
                        layer_updown(layerctl->layersorted[1], layerctl->top - 1);
                        switch_window(layerctl, keyboard_input_layer, layerctl->layersorted[layerctl->top - 1]);
                    }
                }
                if (i == 256 + 0x3b && key_shift !=0) {
                    /* Shitf+F1组合键处理: 强制结束app */
                    struct TASK *task = keyboard_input_layer->task;
                    // 强制结束app时检查tss.ss0, 若为0则代表app未运行, 不能再次结束app.(启动app时会将操作系统段号放入tss.ss0, 结束app时会将tss.ss0置为0)
                    if (task != 0 && task->tss.ss0 != 0) {
                        // 打印信息到控制台
                        console_putstr0(task->console, "\nBreak(key) :\n");
                        // 改变TSS寄存器值时不能切换到其他任务
                        io_cli();
                        // 通过修改tss.eip的值jmp到asm_end_app函数(强制结束app的函数)
                        task->tss.eax = (int) &(task->tss.esp0); // asm_end_app函数所需的参数
                        task->tss.eip = (int) asm_end_app; // 调用asm_end_app函数
                        io_sti();
                        // 唤醒app让其结束自身
                        task_run(task, -1, 0);
                    }
                }
                if (i == 256 + 0x3c && key_shift !=0) {
                    /* Shitf+F2组合键处理: 打开新控制台 */
                    // 控制台图层及任务
                    layer_console = open_console(layerctl, memorytotal);
                    // 控制台显示
                    if (keyboard_input_layer != 0) {
                        layer_slide(layer_console, keyboard_input_layer->vx0 + 20, keyboard_input_layer->vy0 + 20);
                    } else {
                        layer_slide(layer_console, 100, 100);
                    }
                    layer_updown(layer_console, layerctl->top);
                    switch_window(layerctl, keyboard_input_layer, layer_console);
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
                    putfonts8_asc_layer(layer_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 20);
                    // 移动鼠标(此处仅记录坐标, FIFO空闲时再绘制)
                    new_mouse_x = mx;
                    new_mouse_y = my;
                    // 鼠标左键
                    if ((mdec.btn & 0x01) != 0) {
                        if (old_mouse_x < 0) {
                            /* 窗口切换 */
                            /* 从上到下遍历所有图层, 切换到鼠标点击的像素点所属的图层 */
                            int j;
                            for (j = layerctl->top - 1; j > 0; j--) {
                                mouse_click_layer = layerctl->layersorted[j];
                                int x = mx - mouse_click_layer->vx0; // 图层相对坐标x
                                int y = my - mouse_click_layer->vy0; // 图层相对坐标y
                                // 鼠标点击的像素点是否属于该图层
                                if (0 <= x && x < mouse_click_layer->bxsize && 0 <= y && y < mouse_click_layer->bysize) {
                                    // 鼠标点击的像素点是不是透明(跟图层背景颜色一致)
                                    if (mouse_click_layer->buf[y * mouse_click_layer->bxsize + x] != mouse_click_layer->col_inv) {
                                        layer_updown(mouse_click_layer, layerctl->top - 1);
                                        if (mouse_click_layer != keyboard_input_layer) {
                                            /* 鼠标点击的并非当前输入窗口: 输入窗口切换到被点击的窗口 */
                                            switch_window(layerctl, keyboard_input_layer, mouse_click_layer);
                                        }
                                        if (3 <= x && x < mouse_click_layer->bxsize - 3 && 3 <= y && y < 21) {
                                            /* 鼠标点击的是窗口标题栏: 进入窗口拖动模式, 记录下移动前的坐标 */
                                            // 拖动前窗口坐标 
                                            old_window_x = mouse_click_layer->vx0;
                                            old_window_y = mouse_click_layer->vy0;
                                            // 拖动前鼠标坐标
                                            old_mouse_x = mx;
                                            old_mouse_y = my;
                                        }
                                        if (mouse_click_layer->bxsize - 21 <= x && x < mouse_click_layer->bxsize - 5 && 5 <= y && y < 19) {
                                            /* 鼠标点击的是窗口关闭按钮"X"": 关闭窗口 */
                                            if ((mouse_click_layer->flags & 0x10) != 0) {
                                                /* 点击的是app窗口 */
                                                // 切换窗口图层
                                                switch_window(layerctl, keyboard_input_layer, 0);
                                                // TASK地址
                                                struct TASK *task = mouse_click_layer->task;
                                                // 打印信息
                                                console_putstr0(task->console, "\nBreak(key) :\n");
                                                // 改变TSS寄存器值时不能切换到其他任务
                                                io_cli();
                                                // 通过修改tss.eip的值jmp到asm_end_app函数(强制结束app的函数)
                                                task->tss.eax = (int) &(task->tss.esp0); // asm_end_app函数所需的参数
                                                task->tss.eip = (int) asm_end_app; // 调用asm_end_app函数
                                                io_sti();
                                                // 唤醒app让其结束自身
                                                task_run(task, -1, 0);
                                            } else if ((mouse_click_layer->flags & 0x20) != 0) {
                                                /* 点击的是console窗口 */
                                                struct TASK *task = mouse_click_layer->task; // TASK地址
                                                io_cli();
                                                // 调用console的api, 使其关闭自身(若console的app运行中, 中断会被app拦截, console不会被关闭)
                                                fifo32_put(&task->fifo, 4);
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            /* 窗口拖动(此处仅记录坐标, FIFO空闲时或者鼠标左键松开时再绘制窗口拖动) */
                            // x轴移动量保证为4的倍数(先加2能确保向上/向下取整概率相等), 能提高图层性能
                            new_window_x = (old_window_x + mx - old_mouse_x + 2) & ~3;
                            new_window_y = old_window_y + my - old_mouse_y;
                        }
                    } else {
                        /* 没有按下左键, 返回通常模式, 并马上绘制窗口拖动 */
                        old_mouse_x = -1;
                        if (new_window_x != 0x7fffffff) { // 0x7fffffff是默认值, 不为默认值说明有变动
                            layer_slide(mouse_click_layer, new_window_x, new_window_y);
                            new_window_x = 0x7fffffff;
                        }
                    }
                }
            } else if (768 <= i && i <= 1023) {
                /* 通过图层序号关闭控制台 */
                if (keyboard_input_layer != 0) {
                    switch_window(layerctl, keyboard_input_layer, 0);
                }
                close_console(layerctl->layers + (i - 768)); // 关闭指定序号(i-768)的控制台图层
            } else if (1024 <= i && i <= 2023) {
                /* 通过任务序号关闭控制台, 只关闭任务 */
                if (keyboard_input_layer != 0) {
                    switch_window(layerctl, keyboard_input_layer, 0);
                }
                close_console_task(taskctl->tasks + (i - 1024)); // 关闭指定序号(i-768)的控制台图层
            } else if (2024 <= i && i <= 2279) {
                /* 通过图层序号关闭控制台, 只关闭图层 */
                if (keyboard_input_layer != 0) {
                    switch_window(layerctl, keyboard_input_layer, 0);
                }
                // 要关闭的图层
                struct LAYER *layer = layerctl->layers + (i - 2024);
                // 释放图层缓冲区buf
                memory_free_4k(mng, (int) layer->buf, 256 * 165);
                // 释放图层
                layer_free(layer);
            }
        }
    }
}
