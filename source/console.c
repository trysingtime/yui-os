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
    打印字符串(以0结尾)
    - console: 控制台
    - str: 要打印的字符串, 以0结尾
*/
void console_putstr0(struct CONSOLE *console, char *str) {
    for (; *str != 0; str++) {
        console_putchar(console, *str, 1);
    }
    return;
}

/*
    打印字符串(指定长度)
    - console: 控制台
    - str: 要打印的字符串
    - lenth: 要打印的字符串长度
*/
void console_putstr1(struct CONSOLE *console, char *str, int length) {
    int i;
    for (i = 0; i < length; i++) {
        console_putchar(console, str[i], 1);
    }
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
    }  else if (cmdline[0] != 0) {
        /* app指令 */
        int r = cmd_app(console, fat, cmdline);
        if (r == 0) {
            /* 错误指令 */
            // 显示错误提示
            putfonts8_asc_layer(console->layer, 8, console->cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
            // 显示两空白行
            console_newline(console);
            console_newline(console);
        }
    }
    return;
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
    char s[60];
    sprintf(s, "total   %dMB\nfree    %dKB\n\n", memorytotal / (1024 * 1024), free_memory_total(mng) / 1024);
    console_putstr0(console, s);
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
            sprintf(s, "filename.ext    %7d\n", fileinfo[x].size);
            // 文件名
            for (y = 0; y < 8; y++) {
                s[y] = fileinfo[x].name[y];
            }
            // 扩展名
            s[ 9] = fileinfo[x].ext[0];
            s[10] = fileinfo[x].ext[1];
            s[11] = fileinfo[x].ext[2];
            // 绘制
            console_putstr0(console, s);
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
        // 打印文件内容
        console_putstr1(console, p, fileinfo->size);
        // 释放内存
        memory_free_4k(mng, (int) p, fileinfo->size);
    } else {
        /* 没有找到文件的情况 */
        console_putstr0(console, "File not found.\n");
    }
    console_newline(console);
    return;
}

/*
    运行hlt应用
    操作系统执行app:      操作系统将app注册到GDT, 例如段号1003, 然后通过far-Call执行app, app通过far-RET返回
    app调用操作系统API:   操作系统将API注册到IDT, 例如中断号0x40, app通过INT调用API, 操作系统通过IRETD返回
    - console: 运行应用的控制台
    - fat: 解压缩后的FAT信息地址
*/
int cmd_app(struct CONSOLE *console, int *fat, char *cmdline) {
    // 根据输入的指令获取文件名
    char filename[18];
    int i;
    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        filename[i] = cmdline[i];
    }
    filename[i] = 0;
    // 根据文件名(不区分大小写)查找文件
    struct FILEINFO *fileinfo = file_search(filename, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    // 没有找到文件, 自动添加后缀.hrb, 重新寻找
    if (fileinfo == 0 && filename[i - 1] != '.') {
        filename[i] = '.';
        filename[i + 1] = 'H';
        filename[i + 2] = 'R';
        filename[i + 3] = 'B';
        filename[i + 4] = 0;
        fileinfo = file_search(filename, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }
    // 根据文件是否找到,分情况处理
    if (fileinfo != 0) {
        /* 找到文件的情况 */
        // 读取文件内容到*p地址, 使用完后需要释放掉
        struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR; // 内存控制器
        char *p = (char *) memory_alloc_4k(mng, fileinfo->size);
        fiel_loadfile(fileinfo->clustno, fileinfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        // 启动app
        if (fileinfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            // .hrb文件开头的36字节存放了文件信息(详情见README.MD)
            int segment_size    = *((int *) (p + 0x0000)); // 请求操作系统为应用程序准备的数据段的大小
            int esp             = *((int *) (p + 0x000c)); // ESP初值(栈顶, 栈大小由obj2bim参数如(stack:1k)决定)
            int datasize        = *((int *) (p + 0x0010)); // hrb 文件内数据部分的大小
            int datastart       = *((int *) (p + 0x0014)); // hrb 文件内数据部分的起始地址
            // 将app代码段注册到GDT, 段号1003(段号1~2由dsctbl.c使用, 段号3~1002由multitask.c使用), 段属性加上0x60, 将段设置成应用程序专用段
            struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; // GDT地址
            set_segmdesc(gdt + 1003, fileinfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
            // 将app数据段注册到GDT, 段号1004, 大小segment_size, 段属性加上0x60, 将段设置成应用程序专用段
            char *q = (char *) memory_alloc_4k(mng, segment_size);
            *((int *) 0x0fe8) = (int) q; // 将app数据段起始地址放入0x0fe8中, 便于app调用系统API
            set_segmdesc(gdt + 1004, segment_size - 1, (int) q, AR_DATA32_RW + 0x60);
            // 将hrb文件的数据部分复制到数据段(存放在堆栈后面, 因此形成: 数据段=栈+hrb文件数据)
            int i;
            for (i = 0; i < datasize; i++) {
                q[esp + i] = p[datastart + i];
            }
            // 在TSS中注册操作系统的段号和ESP(将操作系统的ESP和段号先后压入TSS栈esp0)
            struct TASK *task = task_current();
            // C语言编写的app需要跳转到.hrb文件0x1b位置(该位置为JMP指令, 会再次跳转到真正的app启动点)
            start_app(0x1b, 1003 * 8, esp, 1004 * 8, &(task->tss.esp0));
            // 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用asm_end_app结束程序直接返回到此处
            // 释放app数据段内存
            memory_free_4k(mng, (int) q, segment_size); // 释放内存
        } else {
            console_putstr0(console, ".hrb file format error.\n");
        }
        // 释放app代码段内存
        memory_free_4k(mng, (int) p, fileinfo->size);
        
        console_newline(console);
        return 1;
    }
    /* 没有找到文件的情况 */
    return 0;  
}

/*
    系统API, 中断INT 0x40触发asm_system_api, asm_system_api组织参数并调用此函数, 根据ebx值调用系统函数
    - edx: 根据此值来判断调用哪个函数
*/
int *system_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    // app数据段起始地址, 此处从0x0fe8获取, 系统运行app时, 已将数据段起始地址放入0x0fe8
    int ds_base = *((int *) 0x0fe8);
    // 控制台内存地址, 此处从0x0fec获取, 控制台初始化时, 已将自身地址放入0x0fec
    struct CONSOLE *console = (struct CONSOLE *) *((int *) 0x0fec);
    /*
        返回值给app
        api执行两次PUSHAD(顺序:EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX), 第一次是调用函数前保存寄存器的值, 第二次是往栈存入参数.
        返回值时, 第二次PUSH的值直接丢弃, 第一次PUSHAD值将会POPAD, 因此app就可以通过寄存器值获取返回值
        此函数的参数为api第二次PUSHAD, 此处要获取第一次PUSHAD的寄存器地址, 根据栈往低位入栈, 因此需要获取高位地址
    */
    int *reg = &eax + 1; // reg[0~7]对应寄存器:EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX
    /* 根据edx的值来判断调用哪个函数 */
    if (edx == 1) {
        /* 1: 显示单个字符 */
        console_putchar(console, eax & 0xff, 1); // eax存放character参数, (eax & 0xff)只保留低8位, 高位全部置0
    } else if (edx == 2) {
        /* 2: 显示字符串(以0结尾) */
        console_putstr0(console, (char *) ebx + ds_base); // ebx传入的是在app数据段中的相对地址, 加上段起始地址得到准确地址
    } else if (edx == 3) {
        /* 3: 显示字符串(指定长度) */
        console_putstr1(console, (char *) ebx + ds_base, ecx); // ebx传入的是在app数据段中的相对地址, 加上段起始地址得到准确地址
    } else if (edx == 4) {
        /* 4: 结束app */
        struct TASK *task = task_current();
        return &(task->tss.esp0); // tss.esp0的地址, start_app()时将操作系统的ESP和段号入栈该esp0, 此时还原(ss:esp), 使指令回到cmd_app(), 从而结束app
    } else if (edx == 5) {
        /* 5: 显示窗口(edx:5,ebx:窗口图层地址,esi:窗口宽度,edi:窗口高度,eax:窗口颜色和透明度,ecx:窗口标题,返回值放入eax) */
        // 新建图层
        struct LAYERCTL *layerctl = (struct LAYERCTL *) *((int *) 0x0fe4); // 图层控制器地址, 操作系统启动时已将地址放入了0x0fe4
        struct LAYER *layer = layer_alloc(layerctl);
        layer_init(layer, (char *) ebx + ds_base, esi, edi, eax);
        // 新建窗口
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        // 显示图层
        layer_slide(layer, 100, 50);
        layer_updown(layer, 6);
        // 返回值
        reg[7] = (int) layer; // 只返回图层地址到EAX寄存器(返回值默认为eax寄存器)
    } else if (edx == 6) {
        /* 6: 窗口显示字符串(edx:6,ebx:窗口图层地址,esi:显示的x坐标,edi:显示的y坐标,eax:颜色,ecx:字符长度,ebp:字符串) */
        struct LAYER *layer = (struct LAYER *) ebx;
        putfonts8_asc(layer->buf, layer->bxsize, esi, edi, eax, (char *) ebp + ds_base);
        layer_refresh(layer, esi, edi, esi + ecx * 8, edi + 16);
    } else if (edx == 7) {
        /* 7: 窗口显示方块(edx:7,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色) */
        struct LAYER *layer = (struct LAYER *) ebx;
        boxfill8(layer->buf, layer->bxsize, ebp, eax, ecx, esi, edi);
        layer_refresh(layer, eax, ecx, esi + 1, edi + 1);
    } else if (edx == 8) {
        /* 8: 初始化app内存控制器(edx:8,ebx:内存控制器地址,eax:管理的内存空间起始地址,ecx:管理的内存空间字节数) */
        memmng_init((struct MEMMNG *) (ebx + ds_base)); // app内存控制器
        ecx &= 0xfffffff0; // 向下取整(0x10为单位)
        memory_free((struct MEMMNG *) (ebx + ds_base), eax, ecx); // 释放app内存(起始位置和大小都由hrb文件头(0x0020,0x0000)决定)
    } else if (edx == 9) {
        /* 9: 分配指定大小的内存(edx:9,ebx:内存控制器地址,eax:分配的内存空间起始地址,ecx:分配的内存空间字节数) */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 向上取整(0x10为单位)
        reg[7] = memory_alloc((struct MEMMNG *) (ebx + ds_base), ecx);
    } else if (edx == 10) {
        /* 10: 释放指定起始地址和大小的内存(edx:10,ebx:内存控制器地址,eax:释放的内存空间起始地址,ecx:释放的内存空间字节数) */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 向上取整(0x10为单位)
        memory_free((struct MEMMNG *) (ebx + ds_base), eax, ecx);
    } else if (edx == 11) {
        /* 11: 在窗口中画点(edx:11,ebx:窗口图层地址,esi:x坐标,edi:y坐标,eax:颜色) */
        struct LAYER *layer = (struct LAYER *) ebx;
        layer->buf[layer->bxsize * edi + esi] = eax;
    } else if (edx == 12) {
        /* 12: 窗口图层刷新(edx:12,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1) */
        struct LAYER *layer = (struct LAYER *) ebx;
        layer_refresh(layer, eax, ecx, esi, edi);
    } else if (edx == 13) {
        /* 13: 窗口绘制直线(edx:13,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色) */
        struct LAYER *layer = (struct LAYER *) ebx;
        api_linewin(layer, eax, ecx, esi, edi, ebp);
    } else if (edx == 14) {
        /* 14: 关闭窗口图层(edx:14,ebx:窗口图层地址) */
        layer_free((struct LAYER *) ebx);
    }
    return 0;
}

/*
    一般保护异常(General Protected Exception)中断处理函数
    - 异常中断(0x00~0x1f): 0x00(除零异常), 0x06(非法指令异常), 0x0c(栈异常), 0x0d(一般保护异常)
    - 在x86架构规范中, 当应用程序试图破坏操作系统或者违背操作系统设置时自动产生0x0d中断
    - 此处仅打印信息, 返回值设置为非0, 让asm_inthandler0d()结束应用程序
    - esp: 64字节(16*int), esp[0~16]值依次为: EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX,DS,ES,错误编号(基本是0),EIP,CS,EFLAGS,ESP(app),SS(app)
            其中esp[10~15]为异常产生时CPU自动PUSH的结果
*/
int *inthandler0d(int *esp) {
    // 控制台内存地址, 此处从0x0fec获取, 控制台初始化时, 已将自身地址放入0x0fec
    struct CONSOLE *console = (struct CONSOLE *) *((int *) 0x0fec);
    console_putstr0(console, "\nINT 0D :\n General Protected Exception.\n");
    // 打印引发异常的指令地址
    char s[30];
    sprintf(s, "EIP = %08X\n", esp[11]);
    console_putstr0(console, s);
    // 强制结束app
    struct TASK *task = task_current();
    return &(task->tss.esp0); // tss.esp0地址在start_app()时将操作系统的ESP和段号入栈, 此时还原, 使指令回到cmd_app(), 从而结束app
}

/*
    栈异常(Stack Exception)中断处理函数
    - 异常中断(0x00~0x1f): 0x00(除零异常), 0x06(非法指令异常), 0x0c(栈异常), 0x0d(一般保护异常)
    - 栈异常只保护操作系统, 禁止app访问自身数据段以外的内存地址, 对数据段内的数据bug不处理
    - 此处仅打印信息, 返回值设置为非0, 让asm_inthandler0c()结束应用程序
    - esp: 64字节(16*int), esp[0~16]值依次为: EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX,DS,ES,错误编号(基本是0),EIP,CS,EFLAGS,ESP(app),SS(app)
        其中esp[10~15]为异常产生时CPU自动PUSH的结果
*/
int *inthandler0c(int *esp) {
    // 控制台内存地址, 此处从0x0fec获取, 控制台初始化时, 已将自身地址放入0x0fec
    struct CONSOLE *console = (struct CONSOLE *) *((int *) 0x0fec);
    console_putstr0(console, "\nINT 0D :\n Stack Exception.\n");
    // 打印引发异常的指令地址
    char s[30];
    sprintf(s, "EIP = %08X\n", esp[11]);
    console_putstr0(console, s);
    // 强制结束app
    struct TASK *task = task_current();
    return &(task->tss.esp0); // tss.esp0地址在start_app()时将操作系统的ESP和段号入栈, 此时还原, 使指令回到cmd_app(), 从而结束app
}

/*
    在窗口绘制直线
    - layer: 窗口所在的图层地址
    - x0,y0,x1,y1: 直线两端的坐标
    - col: 颜色
*/
void api_linewin(struct LAYER *layer, int x0, int y0, int x1, int y1, int col) {
    /* 长边坐标递增1, 短边坐标递增(短边变化量加1/长边变化量加1) */
    // 判断长短边
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (dx < 0) {
        dx = -dx; // 变化量绝对值
    }
    if (dy < 0) {
        dy = -dy; // 变化量绝对值
    }
    int len;
    if (dx >= dy) {
        // 长边为x轴
        len = dx + 1;
        dy = ((dy + 1) << 10) / (dx + 1); // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果
        dx = 1 << 10; // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果
    } else {
        // 长边为y轴
        len = dy + 1;
        dx = ((dx + 1) << 10) / (dy + 1); // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果
        dy = 1 << 10; // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果;
    }
    // 判断方向
    if(x0 > x1) {
        dx = -dx;
    }
    if(y0 > y1) {
        dy = -dy;
    }
    // 绘制直线
    int x = x0 << 10; // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果
    int y = y0 << 10; // 坐标放大1024倍, 后续再缩小1024倍, 实现小数效果

    int i;
    for (i = 0; i < len; i++) {
        layer->buf[(y >> 10) * layer->bxsize + (x >> 10)] = col; // 坐标缩小1024倍, 实现小数效果
        x += dx;
        y += dy;
    }
    return;
}
