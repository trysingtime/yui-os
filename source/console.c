#include "bootpack.h"
#include <stdio.h> // sprintf()
#include <string.h> // strcmp(), strncmp

/*
    控制台任务
*/
void task_console_implement(struct LAYER *layer, unsigned int memorytotal) {
    int i, x, y, cursor_x = 16, cursor_y = 28, cursor_c = -1;
    char s[30], cmdline[30], *p;
    // 内存控制器
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR;
    // 获取当前任务
    struct TASK *task = task_current();
    // 设置中断缓冲区
    int fifobuf[128];
    fifo32_init(&task->fifo, 128, fifobuf, task); // 中断到来自动唤醒task
    // 设置定时器
    struct TIMER *timer;
    timer = timer_alloc(); // 光标闪烁定时器
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50); // 0.05s
    // 显示提示符
    putfonts8_asc_layer(layer, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    /* 获取文件信息 */
    // 磁盘文件信息(0x2600~0x4200)
    struct FILEINFO *fileinfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    // 获取FAT信息
    int *fat = (int *) memory_alloc_4k(mng, 4 * 2880); // 软盘共2880扇区, 每个扇区对应一个FAT信息, 此处使用int(4字节)保存FAT
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200)); // 软盘FAT地址为0x0200~0x13ff

    // 处理中断
    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            //io_stihlt();
            io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 定时器中断处理
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) {
                // 光标闪烁定时器
                // 若为0则显示黑色, 若为1则显示白色, 交替进行, 实现闪烁效果
                if (i != 0)  {
                    timer_init(timer, &task->fifo, 0);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else{
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50); // 再次倒计时0.5s
            }
            // 窗口是否激活
            if (i == 2) {
                // 窗口已激活, 光标闪烁
                cursor_c = COL8_FFFFFF;
            }
            if (i == 3) {
                // 窗口未激活, 光标停止闪烁
                cursor_c = -1;
                // 隐藏光标(显示成背景色白色)
                boxfill8(layer->buf, layer->bxsize, COL8_000000, cursor_x, 28, cursor_x + 7, 43);
            }
            if (256 <= i && i <= 511) {
                // 键盘缓冲区处理(taska发送过来的)
                if (i == 256 + 8) {
                    /* 退格键 */
                    if (cursor_x > 16) {
                        putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1); // 擦除显示的键盘按键
                        cursor_x -= 8; // 光标后移
                    }
                } else if (i == 256 + 10) {
                    /* 回车键 */
                    // 擦除旧行显示的光标
                    putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    // 创建新空白行
                    cursor_y = console_newline(cursor_y, layer);
                    // 获取的指令最后添加0
                    cmdline[cursor_x / 8 - 2] = 0;
                    // 执行指令
                    if (strcmp(cmdline, "mem") == 0) {
                        /* mem指令 */
                        // 绘制内存信息
                        sprintf(s, "total   %dMB", memorytotal / (1024 * 1024));
                        putfonts8_asc_layer(layer, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = console_newline(cursor_y, layer);
                        sprintf(s, "free    %dKB", free_memory_total(mng) / 1024);
                        putfonts8_asc_layer(layer, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = console_newline(cursor_y, layer);
                        cursor_y = console_newline(cursor_y, layer);
                    } else if (strcmp(cmdline, "cls") == 0) {
                        /* cls指令 */
                        // 遍历每一行, 将每个像素涂黑
                        for (y = 28; y < 28 + 128; y++) {
                            for (x = 8; x < 8 + 240; x++) {
                                layer->buf[x + y * layer->bxsize] = COL8_000000;
                            }
                        }
                        // 重绘整个图层
                        layer_refresh(layer, 8, 28, 8 + 240, 28 + 128);
                        // 重置光标位置
                        cursor_y = 28;
                    } else if (strcmp(cmdline, "dir") == 0) {
                        /* dir指令 */
                        // 遍历224个文件信息(0x2600~0x4200只能有224个文件信息(32字节))
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
                                putfonts8_asc_layer(layer, 8 ,cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                cursor_y = console_newline(cursor_y, layer);
                            }
                        }
                        cursor_y = console_newline(cursor_y, layer);
                    } else if (strncmp(cmdline, "type ", 5) == 0) {
                        /* type指令 */
                        // 清空变量, 用以存放文件名
                        for (y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        y = 0;
                        // 获取输入指令中的文件名(type 文件名)(文件名大于8字节无法处理)
                        for (x = 5; y < 11 && cmdline[x] != 0; x++) {
                            if (cmdline[x] == '.' && y <= 8) {
                                // 读取到'.'则认为文件名已取完
                                y = 8;
                            } else {
                                // 文件名转为大写
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    s[y] -= 0x20;
                                }
                                y++;
                            }
                        }
                        // 根据输入指令中的文件名查找文件
                        // 遍历224个文件信息(0x2600~0x4200只能有224个文件信息(32字节))
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
                                for (y = 0; y < 11; y++) {
                                    if (fileinfo[x].name[y] != s[y]) {
                                        // 文件名不一致, 查找下一个文件
                                        break;
                                    }
                                }
                                // 找到文件
                                if (y == 11) {
                                    break;
                                }
                            }
                        }
                        // 根据文件是否找到,分情况处理
                        if (x < 224 && fileinfo[x].name[0] != 0x00) {
                            /* 找到文件的情况 */
                            // 读取文件内容到*p地址, 使用完后需要释放掉
                            p = (char *) memory_alloc_4k(mng, fileinfo[x].size);
                            fiel_loadfile(fileinfo[x].clustno, fileinfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
                            // 打印文件内容(逐字输出)
                            cursor_x = 8;
                            for (y = 0; y < fileinfo[x].size; y++) {
                                s[0] = p[y];
                                s[1] = 0;
                                /* 特殊符号处理 */
                                if (s[0] == 0x09) {
                                    /* 制表符 */
                                    // 制表符用于对其字符, linux每4个字符位置一个制表位, windows则是8个字符, 此处取4个字符
                                    for (;;) {
                                        putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                                        cursor_x += 8;
                                        if (cursor_x == 8 + 240) {
                                            cursor_x = 8;
                                            cursor_y = console_newline(cursor_y, layer);
                                        }
                                        if (((cursor_x - 8) & 0x1f) == 0) {
                                            // 4个字符宽度(4*8=32像素)为一个制表位, 因此能被32除余数为0(低5位为0)的位置为制表位, 制表符到此结束
                                            break;
                                        }
                                    }
                                } else if (s[0] == 0x0a) {
                                    /* 换行符 */
                                    // linux换行符为0x0a, windows换行符为0x0d 0x0a(先回车再换行), 此处我们统一为0x0a, 两个系统都兼容, 这样回车符就忽略了
                                    cursor_x = 8;
                                    cursor_y = console_newline(cursor_y, layer);
                                } else if (s[0] == 0x0d) {
                                    /* 回车符 */
                                    // linux换行符为0x0a, windows换行符为0x0d 0x0a(先回车再换行), 此处我们统一为0x0a, 两个系统都兼容, 这样回车符就忽略了
                                } else {
                                    /* 普通字符 */
                                    putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240) {
                                        // 到达最右端换行
                                        cursor_x = 8;
                                        cursor_y = console_newline(cursor_y, layer);
                                    }
                                }
                            }
                            // 释放内存
                            memory_free_4k(mng, (int) p, fileinfo[x].size);
                        } else {
                            /* 没有找到文件的情况 */
                            putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
                            cursor_y = console_newline(cursor_y, layer);
                        }
                        cursor_y = console_newline(cursor_y, layer);
                    } else if (cmdline[0] != 0) {
                        /* 错误指令 */
                        // 显示错误提示
                        putfonts8_asc_layer(layer, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
                        // 显示两空白行
                        cursor_y = console_newline(cursor_y, layer);
                        cursor_y = console_newline(cursor_y, layer);
                    }
                    putfonts8_asc_layer(layer, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1); // 显示新一行提示符
                    cursor_x = 16; // 光标重置到开头       
                } else {
                    /* 普通字符 */
                    if (cursor_x < 240) {
                        s[0] = i - 256;
                        s[1] = 0;
                        // 获取输入的指令
                        cmdline[cursor_x / 8 - 2] = i - 256;
                        // 显示键盘按键
                        putfonts8_asc_layer(layer, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        // 光标前移
                        cursor_x += 8;
                    }
                }
            }
            // 重绘光标
            // 是否显示光标
            if (cursor_c >= 0) {
                boxfill8(layer->buf, layer->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15); // 显示白色
            }
            layer_refresh(layer, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16); // 刷新图层                
        }
    }
}

/*
    创建新空白行
*/
int console_newline(int cursor_y, struct LAYER *layer) {
    if (cursor_y < 28 + 112) {
        // 光标换行
        cursor_y += 16;
    } else {
        // 窗口滚动
        // 遍历每一行, 将每个像素向上移动一行(16像素距离)
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
    return cursor_y;
}
