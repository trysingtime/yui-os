#include "bootpack.h"
#include <stdio.h> // sprintf()
#include <string.h> // strcmp(), strncmp

/*
    控制台任务
*/
void console_task(struct LAYER *layer, unsigned int memorytotal) {
    /* 初始化控制台 */
    struct CONSOLE console;
    console.layer = layer;
    console.cursor_x = 8;
    console.cursor_y = 28;
    console.cursor_color = -1; // 颜色为-1, 不显示
    // API: 将控制台内存地址放入0x0fec中, 应用程序可以通过0x0fec获取控制台地址, 进而调用控制台函数
    *((int *) 0x0fec) = (int) &console;

    /* 获取当前任务 */
    struct TASK *task = task_current();
    /* 设置中断缓冲区 */
    int fifobuf[128];
    fifo32_init(&task->fifo, 128, fifobuf, task); // 中断到来自动唤醒task
    /* 设置定时器 */
    struct TIMER *timer;
    timer = timer_alloc(); // 光标闪烁定时器
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50); // 0.05s

    /* 获取FAT信息, 用于文件相关的命令 */
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR; // 内存控制器
    int *fat = (int *) memory_alloc_4k(mng, 4 * 2880); // 软盘共2880扇区, 每个扇区对应一个FAT信息, 此处使用int(4字节)保存FAT
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200)); // 软盘FAT地址为0x0200~0x13ff

    /* 显示提示符 */
    console_putchar(&console, '>', 1);
    
    /* 存放输入的指令 */
    char cmdline[30];

    /* 处理中断 */
    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            //io_stihlt();
            io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 定时器中断处理
            int i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) {
                // 光标闪烁定时器
                // 若为0则显示黑色, 若为1则显示白色, 交替进行, 实现闪烁效果
                if (i != 0)  {
                    timer_init(timer, &task->fifo, 0);
                    if (console.cursor_color >= 0) {
                        console.cursor_color = COL8_FFFFFF;
                    }
                } else{
                    timer_init(timer, &task->fifo, 1);
                    if (console.cursor_color >= 0) {
                        console.cursor_color = COL8_000000;
                    }
                }
                timer_settime(timer, 50); // 再次倒计时0.5s
            }
            // 窗口是否激活
            if (i == 2) {
                // 窗口已激活, 光标闪烁
                console.cursor_color = COL8_FFFFFF;
            }
            if (i == 3) {
                // 窗口未激活, 光标停止闪烁
                console.cursor_color = -1;
                // 隐藏光标(显示成背景色白色)
                boxfill8(layer->buf, layer->bxsize, COL8_000000, console.cursor_x, 28, console.cursor_x + 7, 43);
            }
            if (256 <= i && i <= 511) {
                // 键盘缓冲区处理(taska发送过来的)
                if (i == 256 + 8) {
                    /* 退格键 */
                    if (console.cursor_x > 16) {
                        // 使用空格擦除当前光标
                        console_putchar(&console, ' ', 0);
                        // 光标前移
                        console.cursor_x -= 8;
                    }
                } else if (i == 256 + 10) {
                    /* 回车键 */
                    // 使用空格擦除当前光标
                    console_putchar(&console, ' ', 0);
                    // 获取的指令最后添加0
                    cmdline[console.cursor_x / 8 - 2] = 0;
                    // 创建新空白行
                    console_newline(&console);
                    // 执行指令
                    console_runcmd(cmdline, &console, fat, memorytotal);
                    // 显示新一行提示符
                    console_putchar(&console, '>', 1);
                } else {
                    /* 普通字符 */
                    if (console.cursor_x < 240) {
                        // 将输入的每个字符都放入cmdline
                        cmdline[console.cursor_x / 8 - 2] = i - 256;
                        // 显示输入的字符
                        console_putchar(&console, i - 256, 1);
                    }
                }
            }
            // 重绘光标
            if (console.cursor_color >= 0) {
                // 是否显示光标
                boxfill8(layer->buf, layer->bxsize, console.cursor_color, console.cursor_x, console.cursor_y, console.cursor_x + 7, console.cursor_y + 15); // 显示白色
            }
            layer_refresh(layer, console.cursor_x, console.cursor_y, console.cursor_x + 8, console.cursor_y + 16); // 刷新图层                
        }
    }
}

/*
    在控制台显示字符
    - console: 控制台
    - character: 要显示的字符
    - move: 显示字符后光标是否后移
*/
void console_putchar(struct CONSOLE *console, int character, char move) {
    char s[2];
    s[0] = character;
    s[1] = 0;
    /* 特殊符号处理 */
    if (s[0] == 0x09) {
        /* 制表符 */
        // 制表符用于对其字符, linux每4个字符位置一个制表位, windows则是8个字符, 此处取4个字符
        for (;;) {
            // 循环绘制空格
            putfonts8_asc_layer(console->layer, console->cursor_x, console->cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
            console->cursor_x += 8;
            if (console->cursor_x == 8 + 240) {
                // 到达最右端换行
                console_newline(console);
            }
            if (((console->cursor_x - 8) & 0x1f) == 0) {
                // 4个字符宽度(4*8=32像素)为一个制表位, 因此能被32除余数为0(低5位为0)的位置为制表位, 制表符到此结束
                break;
            }
        }
    } else if (s[0] == 0x0a) {
        /* 换行符 */
        // linux换行符为0x0a, windows换行符为0x0d 0x0a(先回车再换行), 此处我们统一为0x0a, 两个系统都兼容, 这样回车符就忽略了
        console_newline(console);
    } else if (s[0] == 0x0d) {
        /* 回车符 */
        // linux换行符为0x0a, windows换行符为0x0d 0x0a(先回车再换行), 此处我们统一为0x0a, 两个系统都兼容, 这样回车符就忽略了
    } else {
        /* 普通字符 */
        // 绘制字符
        putfonts8_asc_layer(console->layer, console->cursor_x, console->cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
        // 显示字符后光标是否后移
        if (move != 0) {
            console->cursor_x += 8;
            if (console->cursor_x == 8 + 240) {
                // 到达最右端换行
                console_newline(console);
            }
        }
    }
    return;
}

/*
    创建新空白行
    - consoel: 控制台
*/
void console_newline(struct CONSOLE *console) {
    if (console->cursor_y < 28 + 112) {
        // 光标换行
        console->cursor_y += 16;
    } else {
        // 窗口滚动
        // 遍历每一行, 将每个像素向上移动一行(16像素距离)
        struct LAYER *layer = console->layer;
        int x, y;
        for (y = 28; y < 28 + 112; y++) {
            for (x = 8; x < 8 + 240; x++) {
                layer->buf[x + y * layer->bxsize] = layer->buf[x + (y + 16) * layer->bxsize];
            }
        }
        // 将最后一行每个像素涂成黑色
        for (y = 28 + 112; y < 28 + 128; y++) {
            for (x = 8; x < 8 + 240; x++) {
                layer->buf[x + y * layer->bxsize] = COL8_000000;
            }
        }
        // 重绘整个图层
        layer_refresh(layer, 8, 28, 8 + 240, 28 + 128);
    }
    // 光标重置到行首
    console->cursor_x = 8;
    return;
}

/*
    执行控制台输入的指令
    - cmdline: 输入到控制台的指令
    - console: 执行指令的控制台
    - fat: 解压缩后的FAT信息地址
    - memorytotal: 系统总内存
*/
void console_runcmd(char *cmdline, struct CONSOLE *console, int *fat, unsigned int memorytotal) {
    if (strcmp(cmdline, "mem") == 0) {
        /* mem指令 */
        cmd_mem(console, memorytotal);
    } else if (strcmp(cmdline, "cls") == 0) {
        /* cls指令 */
        cmd_cls(console);
    } else if (strcmp(cmdline, "dir") == 0) {
        /* dir指令 */
        cmd_dir(console);
    } else if (strncmp(cmdline, "type ", 5) == 0) {
        /* type指令 */
        cmd_type(console, fat, cmdline);
    } else if (strcmp(cmdline, "hlt") == 0) {
        /* 运行指定程序hlt */
        app_hlt(console, fat);
    } else if (cmdline[0] != 0) {
        /* 错误指令 */
        // 显示错误提示
        putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
        // 显示两空白行
        console_newline(console);
        console_newline(console);
    }
}

/*
    mem指令, 查询当前内存情况
    - console: 执行指令的控制台
    - memorytotal: 系统总内存
*/
void cmd_mem(struct CONSOLE *console, unsigned int memorytotal) {
    // 内存控制器
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    // 绘制内存信息
    char s[30];
    sprintf(s, "total   %dMB", memorytotal / (1024 * 1024));
    putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
    console_newline(console);

    sprintf(s, "free    %dKB", free_memory_total(mng) / 1024);
    putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
    console_newline(console);
    console_newline(console);
    return;
}

/*
    cls指令, 清屏
    - console: 执行指令的控制台
*/
void cmd_cls(struct CONSOLE *console) {
    struct LAYER *layer = console->layer;
    // 遍历每一行, 将每个像素涂黑
    int x,y;
    for (y = 28; y < 28 + 128; y++) {
        for (x = 8; x < 8 + 240; x++) {
            layer->buf[x + y * layer->bxsize] = COL8_000000;
        }
    }
    // 重绘整个图层
    layer_refresh(layer, 8, 28, 8 + 240, 28 + 128);
    // 重置光标位置
    console->cursor_y = 28;
    return;
}

/*
    dir指令, 显示文件信息
    - console: 执行指令的控制台
*/
void cmd_dir(struct CONSOLE *console) {
    // 磁盘文件信息(0x2600~0x4200)
    struct FILEINFO *fileinfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    // 遍历224个文件信息(0x2600~0x4200只能有224个文件信息(32字节))
    int x,y;
    for (x = 0; x < 224; x++) {
        if (fileinfo[x].name[0] == 0x00) {
            // 文件名第一个字节为0x00代表这一段不包含任何文件名信息
            break;
        }
        if (fileinfo[x].name[0] == 0xe5) {
            // 文件名第一个字节为0xe代表这个文件已被删除
            break;
        }
        // type(文件属性): 一般0x20/0x00,0x01(只读文件),0x02(隐藏文件),0x04(系统文件),0x08(非文件信息,如磁盘名称),0x10目录
        if ((fileinfo[x].type & 0x18) == 0) {
            // 非目录且非"非文件信息"
            // 打印文件大小
            char s[30];
            sprintf(s, "filename.ext    %7d", fileinfo[x].size);
            // 文件名
            for (y = 0; y < 8; y++) {
                s[y] = fileinfo[x].name[y];
            }
            // 扩展名
            s[ 9] = fileinfo[x].ext[0];
            s[10] = fileinfo[x].ext[1];
            s[11] = fileinfo[x].ext[2];
            // 绘制
            putfonts8_asc_layer(console->layer, 8 ,console->cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
            console_newline(console);
        }
    }
    console_newline(console);
    return;   
}

/*
    type指令, 打印指定文件内容
    - console: 执行指令的控制台
    - fat: 解压缩后的FAT信息地址
    - cmdline: 输入到控制台的指令
*/
void cmd_type(struct CONSOLE *console, int *fat, char *cmdline) {
    // 根据文件名查找文件
    struct FILEINFO *fileinfo = file_search(cmdline + 5, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    // 根据文件是否找到,分情况处理
    if (fileinfo != 0) {
        /* 找到文件的情况 */
        // 读取文件内容到*p地址, 使用完后需要释放掉
        struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR; // 内存控制器
        char *p = (char *) memory_alloc_4k(mng, fileinfo->size);
        fiel_loadfile(fileinfo->clustno, fileinfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        // 打印文件内容(逐字输出)
        int i;
        for (i = 0; i < fileinfo->size; i++) {
            console_putchar(console, p[i], 1);
        }
        // 释放内存
        memory_free_4k(mng, (int) p, fileinfo->size);
    } else {
        /* 没有找到文件的情况 */
        putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
        console_newline(console);
    }
    console_newline(console);
    return;
}

/*
    运行hlt应用
    - console: 运行应用的控制台
    - fat: 解压缩后的FAT信息地址
*/
void app_hlt(struct CONSOLE *console, int *fat) {
    // 根据文件名查找文件
    struct FILEINFO *fileinfo = file_search("HLT.HRB", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    // 根据文件是否找到,分情况处理
    if (fileinfo != 0) {
        /* 找到文件的情况 */
        // 读取文件内容到*p地址, 使用完后需要释放掉
        struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR; // 内存控制器
        char *p = (char *) memory_alloc_4k(mng, fileinfo->size);
        fiel_loadfile(fileinfo->clustno, fileinfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        // 将hlt.hrb注册到GDT, 段号1003(段号1~2由dsctbl.c使用, 段号3~1002由multitask.c使用)
        struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; // GDT地址
        set_segmdesc(gdt + 1003, fileinfo->size - 1, (int) p, AR_CODE32_ER);
        // 使用far-Call跨段调用应用函数(段号1003), 因此应用函数上要相应使用far-RET回应
        farcall(0, 1003 * 8);
        // 释放内存
        memory_free_4k(mng, (int) p, fileinfo->size);
    } else {
        /* 没有找到文件的情况 */
        putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
        console_newline(console);
    }
    console_newline(console);
    return;  
}
