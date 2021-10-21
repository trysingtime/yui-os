/*
    文本浏览
    示例: notepad ipl20.nas -w100 -h30 -t4
    命令行选项: -w窗口宽度, -h窗口高度, -tTAB宽度
    按键:a~f:纵向滚动速度; A~F:横向滚动速度; <或>调节TAB宽度; q或Q:退出
*/
#include <stdio.h>
#include "../../api/api.h"

/*
    字符串转整数
    - s: 字符串地址
    - endp: 转换字符串时所读取到的字符串末尾地址的地址(一般为0)
    - base: 进制(0代表自动识别, 0x开头识别为十六进制, 0开头识别为八进制)
*/
int strtol(char *s, char **endp, int base); /* 标准函数(stdlib.h) */
char *skipspace(char *p);
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang);
char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang);
int puttab(int x, int w, int xskip, char *s, int tab);

void HariMain(void) {
    /* 默认的参数 */
    int w = 30; /* 窗口宽度 */
    int h = 10; /* 窗口高度 */
    int t = 4; /* TAB宽度 */
    int xskip = 0; /* 文本横向偏移大小 */
    int spd_x = 1; /* 横向滚动行数 */
    int spd_y = 1; /* 纵向滚动行数 */
    char *q = 0; /* 存放指令中的文件名 */
    char *r = 0; /* 存放指令中的文件名后剩余部分 */

    /* 解析输入的指令 */
    // 获取输入的指令
    char s[30];
    api_cmdline(s, 30);
    // 跳到第一个空格
    char *p;
    for (p = s; *p > ' '; p++) {}
    for (; *p != 0; ) {
        p = skipspace(p);
        if (*p == '-') {
            /* 命令行选项(-w/-h/-t) */
            if (p[1] == 'w') {
                /* 窗口宽度 */
                w = strtol(p + 2, &p, 0);
                if (w < 20) {
                    w = 20;
                }
                if (w > 126) {
                    w = 126;
                }
            } else if (p[1] == 'h') {
                /* 窗口高度 */
                h = strtol(p + 2, &p, 0);
                if (h < 1) {
                    h = 1;
                }
                if (h > 45) {
                    h = 45;
                }
            } else if (p[1] == 't') {
                /* TAB宽度 */
                t = strtol(p + 2, &p, 0);
                if (t < 1) {
                    t = 4;
                }
            } else {
err:
                api_putstr0(" >notepad file [-w30 -h10 -t4]\n");
                api_end();
            }
        } else {
            /* 命令行文件名 */
            // 将文件名放到q
            if (q != 0) {
                goto err;
            }
            q = p;
            for (; *p > ' '; p++) {}
            // 将文件名后剩余部分放到r
            r = p;
        }
    }
    // 读取文件名失败
    if (q == 0) {
        goto err;
    }

    // 读取文件名成功, 创建窗口
    char winbuf[1024 * 757];
    int win = api_openwin(winbuf, w * 8 + 16, h * 16 + 37, -1, "notepad");
    api_boxfillwin(win, 6, 27, w * 8 + 9, h * 16 + 30, 7);
    // 载入文件
    *r = 0;
    int i = api_fopen(q); // 文件内容
    if (i == 0) {
        api_putstr0("file open error.\n");
        api_end();
    }
    // 限制文件大小
    int j = api_fsize(i, 0); // 文件大小
    if (j >= 240 * 1024 - 1) {
        j = 240 * 1024 - 2;
    }
    // 读取文件
    char txtbuf[240 * 1024];
    txtbuf[0] = 0x0a; /* 文本哨兵, 至少有一个换行符 */
    api_fread(txtbuf + 1, j, i);
    txtbuf[j + 1] = 0;
    // 关闭文件
    api_fclose(i);
    // 删掉文件中所有0x0d(回车), 将windows回车换行转换成linux换行
    q = txtbuf + 1;
    for (p = txtbuf + 1; *p != 0; p++) {
        if (*p != 0x0d) {
            *q = *p;
            q++;
        }
    }
    *q = 0;

    //显示
    p = txtbuf + 1;
    int lang = api_getlang(); /* 当前语言模式 */
    for (;;) {
        // 显示文件文本
        textview(win, w, h, xskip, p, t, lang);
        // 按键操作(a~f:纵向滚动速度; A~F:横向滚动速度; <或>调节TAB宽度; q或Q:退出)
        i = api_getkey(1);
        if (i == 'Q' || i == 'q') {
            api_end();
        }
        if ('A' <= i && i <= 'F') {
            spd_x = 1 << (i - 'A');
        }
        if ('a' <= i && i <= 'f') {
            spd_y = 1 << (i - 'a');
        }
        if (i == '<' && t > 1) {
            t /= 2;
        }
        if (i == '>' && t < 256) {
            t *= 2;
        }
        if (i == '4') {
            for (;;) {
                xskip -= spd_x;
                if (xskip < 0) {
                    xskip = 0;
                }
                if (api_getkey(0) != '4') {
                    break;
                }
            }
        }
        if (i == '6') {
            for (;;) {
                xskip += spd_x;
                if (api_getkey(0) != '6') {
                    break;
                }
            }
        }
        if (i == '8') {
            for (;;) {
                for (j = 0; j < spd_y; j++) {
                    if (p == txtbuf + 1) {
                        break;
                    }
                    // 回到上一个字符为0x0a(换行)
                    for (p--; p[-1] != 0x0a; p--) {}
                }
                if (api_getkey(0) != '8') {
                    break;
                }
            }
        }
        if (i == '2') {
            for (;;) {
                for (j = 0; j < spd_y; j++) {
                    for (q = p; *q != 0 && *q != 0x0a; q++) {}
                    if (*q == 0) {
                        break;
                    }
                    p = q + 1;
                }
                if (api_getkey(0) != '2') {
                    break;
                }
            }
        }
    }
}

/*
    跳过字符串最前面的空格
    - p: 字符串地址
*/
char *skipspace(char *p) {
    for (; *p == ' '; p++) {}
    return p;
}

/*
    显示文件文本
    - win: 窗口地址
    - w: 窗口宽度
    - y: 绘制的y坐标
    - xskip: 文本横向偏移大小
    - p: 要绘制的文件内容起始地址
    - tab: TAB宽度
    - lang: 语言模式
*/
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang) {
    api_boxfillwin(win + 1, 8, 29, w * 8 + 7, h * 16 + 28, 7); // 优化性能, win+1表示不刷新图层, 最后一块刷新
    int i;
    for (i = 0; i < h; i++) {
        p = lineview(win, w, i * 16 + 29, xskip, p, tab, lang);
    }
    api_refreshwin(win, 8, 29, w * 8 + 8, h * 16 + 29);
    return;
}

/*
    绘制一行文件内容
    - win: 窗口地址
    - w: 窗口宽度
    - y: 绘制的y坐标
    - xskip: 文本横向偏移大小
    - p: 要绘制的文件内容起始地址
    - tab: TAB宽度
    - lang: 语言模式
*/
char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang) {
    int x = -xskip; // 文本横向偏移大小, 字符位置小于该偏移量的不显示
    // 将文本转换为可打印的字符串并放入s[130]
    char s[130];
    for (;;) {
        if (*p == 0) {
            /* 文件结束 */
            break;
        }
        if (*p == 0x0a) {
            /* 换行 */
            p++;
            break;
        }
        if (lang == 0) {
            /* 英文模式 */
            if (*p == 0x09) {
                /* 制表符 */
                x = puttab(x, w, xskip, s, tab);
            } else {
                /* 普通字符 */
                if ( 0 <= x && x < w) {
                    s[x] = *p;
                }
                x++;
            }
            p++;
        }
        if (lang == 1 || lang == 3) {
            /* 中文GB2312 或者日语EUC */
            if (*p == 0x09) {
                /* 制表符 */
                x = puttab(x, w, xskip, s, tab);
                p++;
            } else if (0xa1 <= *p && *p <= 0xfe) {
                /* 全角字符 */
                if (x == -1) {
                    /* 左半部分全角符号无法显示 */
                    s[0] = ' ';
                }
                if ( 0 <= x && x < w - 1) {
                    s[x] = *p;
                    s[x + 1] = p[1];
                }
                if (x == w - 1) {
                    /* 右半部分全角符号无法显示 */
                    s[x] = ' ';
                }
                x += 2;
                p += 2;
            } else {
                /* 半角字符 */
                if ( 0 <= x && x < w) {
                    s[x] = *p;
                }
                x++;
                p++;
            }
        }
        if (lang == 2) {
            /* 日语Shift-JIS */
            if (*p == 0x09) {
                /* 制表符 */
                x = puttab(x, w, xskip, s, tab);
                p++;
            } else if ((0x81 <= *p && *p <= 0x9f) || (0xe0 <= *p && *p <= 0xfc)) {
                /* 全角字符 */
                if (x == -1) {
                    /* 左半部分全角符号无法显示 */
                    s[0] = ' ';
                }
                if ( 0 <= x && x < w - 1) {
                    s[x] = *p;
                    s[x + 1] = p[1];
                }
                if (x == w - 1) {
                    /* 右半部分全角符号无法显示 */
                    s[x] = ' ';
                }
                x += 2;
                p += 2;
            } else {
                /* 半角字符 */
                if ( 0 <= x && x < w) {
                    s[x] = *p;
                }
                x++;
                p++;
            }
        }
    }
    // 绘制已转化的字符串
    if (x > w) {
        x = w;
    }
    if (x > 0) {
        s[x] = 0;
        api_putstrwin(win + 1, 8, y, 0, x, s);
    }
    return p;
}

/*
    将制表符转换为空格
    - x: 当前字符位置
    - w: 当前窗口宽度
    - xskip: 文本横向偏移大小
    - s: 存放结果
    - tab: 制表符宽度
*/
int puttab(int x, int w, int xskip, char *s, int tab) {
    for (;;) {
        // 当前字符位置不在窗口时不显示
        if (0 <= x && x < w) {
            s[x] = ' ';
        }
        x++;
        if ((x + xskip) % tab == 0) {
            break;
        }
    }
    return x;
}
