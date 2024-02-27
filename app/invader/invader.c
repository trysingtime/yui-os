/* 打星星游戏 */
#include <stdio.h> /* sprintf */
#include <string.h> /* strlen */
#include "../../api/api.h"

void putstr(int win, char *winbuf, int x, int y, int color, unsigned char *s);
void wait(int i, int timer, char *keyflag);

// 使用专门的字体绘制飞机/敌人/炮弹
static unsigned char charset[16 * 8] = {
    /* invader:"abcd", fighter:"efg", laser:"h" */

	/* invader(0) */
	0x00, 0x00, 0x00, 0x43, 0x5f, 0x5f, 0x5f, 0x7f,
	0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x20, 0x3f, 0x00,

	/* invader(1) */
	0x00, 0x0f, 0x7f, 0xff, 0xcf, 0xcf, 0xcf, 0xff,
	0xff, 0xe0, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0x00,

	/* invader(2) */
	0x00, 0xf0, 0xfe, 0xff, 0xf3, 0xf3, 0xf3, 0xff,
	0xff, 0x07, 0xff, 0xff, 0x03, 0x03, 0x03, 0x00,

	/* invader(3) */
	0x00, 0x00, 0x00, 0xc2, 0xfa, 0xfa, 0xfa, 0xfe,
	0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x04, 0xfc, 0x00,

	/* fighter(0) */
	0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x43, 0x47, 0x4f, 0x5f, 0x7f, 0x7f, 0x00,

	/* fighter(1) */
	0x18, 0x7e, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xff,
	0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x00,

	/* fighter(2) */
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0xc2, 0xe2, 0xf2, 0xfa, 0xfe, 0xfe, 0x00,

	/* laser */
	0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00
};

void HariMain(void) {
    // 创建窗口
    char winbuf[336 * 261];
    int win = api_openwin(winbuf, 336, 261, -1, "invader");
    api_boxfillwin(win, 6, 27, 329, 254, 0);
    // 创建定时器
    int timer = api_alloctimer();
    api_inittimer(timer, 128);

    // 游戏参数
    int high; // 最高分数
    int score; // 当前分数
    int point; // 得分
    int movewait; // 当这个变量变为0时敌人前进一步
    int movewait0; // movewait初始值(消灭30只敌人后减少)
    // 飞机
    int fx; // 飞机坐标x
    char keyflag[4]; // 飞机动作指令
    int lx, ly; // 炮弹坐标
    int laserwait; // 炮弹充能时间
    // 敌人
    char invstr[32 * 6]; // 最多6行敌人, 每行敌人5个, 使用专门字体(abcd)表示一个敌人, 每行敌人32字节
    static char invstr0[32] = " abcd abcd abcd abcd abcd "; // 使用专门字体表示一行敌人, 共32字节
    int invline; // 敌人行数
    int ix, iy; // 敌人坐标
    int idir; // 敌人移动方向

    // 初始化游戏参数
    high = 0;
    putstr(win, winbuf, 22, 0, 7, "HIGH:00000000");
restart:
    score = 0;
    putstr(win, winbuf, 4, 0, 7, "SCORE:00000000");
    point = 1;
    movewait0 = 20;
    // 飞机
    fx = 18;
    putstr(win, winbuf, fx, 13, 6, "efg"); // 使用专门字体(efg)绘制飞机
    // wait(100, timer, keyflag); // 延时0.1s
    
next_group:
    // wait(100, timer, keyflag); // 延时0.1s
    // 敌人
    invline = 6;
    ix = 7;
    iy = 1;
    idir = +1;
    movewait = movewait0;
    // 一行行绘制敌人
    int i,j;
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 27; j++) {
            invstr[i * 32 + j] = invstr0[j];
        }
        putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);
    }
    // 飞机
    keyflag[0] = 0;
    keyflag[1] = 0;
    keyflag[2] = 0;
    lx = 0;
    ly = 0;
    laserwait = 0;
    // wait(100, timer, keyflag); // 延时0.1s

    for (;;) {
        // 飞机
        if (laserwait != 0) {
            /* 判断炮弹是否充能完毕 */
            laserwait--;
            keyflag[2] = 0;
        }
        // 延时0.04s并获取飞机动作指令
        wait(4, timer, keyflag);
        if (keyflag[0] != 0 && fx > 0) {
            /* 飞机左移 */
            fx--;
            putstr(win, winbuf, fx, 13, 6, "efg "); // 右边空白
            keyflag[0] = 0;
        }
        if (keyflag[1] != 0 && fx < 37) {
            /* 飞机右移 */
            putstr(win, winbuf, fx, 13, 6, " efg"); // 左边空白
            fx++;
            keyflag[1] = 0;
        }
        if (keyflag[2] != 0 && laserwait == 0) {
            /* 发射炮弹 */
            laserwait = 15;
            lx = fx + 1;
            ly = 13;
        }

        // 敌人
        if (movewait != 0) {
            /* 敌人不动 */
            movewait--;
        } else {
            /* 敌人移动 */
            movewait = movewait0;
            if (ix + idir > 14 || ix + idir < 0) {
                /* 左右平移到边界时, 颠倒平移方向, 且前进一行 */
                if (iy + invline == 13) {
                    /* GAME OVER */
                    break;
                }
                // 颠倒平移方向
                idir = - idir;
                // 擦除最上面一行敌人
                putstr(win, winbuf, ix + 1, iy, 0, "                         ");
                // 敌人前进一行(后续再绘制)
                iy++;
            } else {
                /* 左右平移 */
                ix += idir;
            }
            // 重新一行行绘制敌人
            for (i = 0; i < invline; i++) {
                putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);
            }
        }

        // 炮弹
        if (ly > 0) {
            /* 炮弹移动前重绘原位置 */
            if (ly < 13) {
                if (ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline) {
                    /* 原位置为敌人, 重绘敌人 */
                    putstr(win, winbuf, ix, iy, 2, invstr + (ly - iy) * 32);
                } else {
                    /* 原位置为空白, 重绘空白 */
                    putstr(win, winbuf, lx, ly, 0, " ");
                }
            }
            // 炮弹前进
            ly--;
            if (ly > 0) {
                /* 炮弹移动后绘制新位置 */
                putstr(win, winbuf, lx, ly, 3, "h");
            } else {
                /* 炮弹到达顶部减少得分 */
                point -= 10;
                if (point <= 0) {
                    point = 1;
                }
            }
            // 炮弹进入敌群
            if (ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline) {
                char *p = invstr + (ly - iy) * 32 + (lx - ix); // 炮弹命中的敌群vram地址
                if (*p != ' ') {
                    /* 炮弹命中敌人 */
                    // 更新当前分数
                    score += point;
                    char s[12];
                    sprintf(s, "%08d", score);
                    putstr(win, winbuf, 10, 0, 7, s);
                    // 增加得分
                    point++;
                    // 更新最高分数
                    if (high < score) {
                        high = score;
                        putstr(win, winbuf, 27, 0, 7, s);
                    }
                    // 擦除整个敌人(4字节)
                    for (p--; *p != ' '; p--) {}
                    for (i = 1; i < 5; i++) {
                        p[i] = ' ';
                    }
                    putstr(win, winbuf, ix, ly, 2, invstr + (ly - iy) * 32); // 重绘整行敌人
                    // 遍历所有敌人, 检查敌人是否全部被消灭
                    for(; invline > 0; invline--) {
                        for (p = invstr + (invline - 1) * 32; *p != 0; p++) {
                            if (*p != ' ') {
                                /* 仍有敌人存活, 当前炮弹消失, 游戏继续 */
                                goto alive;
                            }
                        }
                    }
                    /* 没有敌人存活, 难度升级, 重新部署新敌群 */
                    movewait0 -= movewait0 / 3; // 敌人移动速度提高
                    goto next_group;
alive:                    
                    // 炮弹碰撞后消失
                    ly = 0;
                }
            }
        }
    }
    /* GAME OVER: 等待Enter键, 按键后清屏并重新开始游戏 */
    putstr(win, winbuf, 15, 6, 1, "GAME OVER"); // 打印"GAME OVER"
    wait(0, timer, keyflag); // 等待Enter键
    for (i = 1; i < 14; i++) {
        /* 清屏 */
        putstr(win, winbuf, 0, i, 0, "                                        ");
    }
    goto restart;
}

/*
    绘制字符串(遇到0停止, a~h字符使用专门字体绘制, 其他字符使用操作系统api绘制)
    win: 窗口图层地址
    winbuf: 窗体内容起始地址
    x, y: 字符串位置(字符(8*16像素)位置而非坐标)
    color: 色号
    s: 字符串
*/
void putstr(int win, char *winbuf, int x, int y, int color, unsigned char *s) {
    // 擦除原有字符
    x = x * 8 + 8; // 计算x轴坐标(窗口左边界3像素+内容边框4像素+间距2像素-1=8像素)
    y = y * 16 + 29; // 计算y轴坐标(窗口顶部边界3像素+标题栏18像素+内容边框7像素+间距2像素-1=29像素)
    int x0 = x; // 保存初始x值
    int i = strlen(s); // 计算字符串s的字符数
    api_boxfillwin(win + 1, x, y, x + i * 8 - 1, y + 15, 0); // 优化性能, win+1表示不刷新图层, 最后一块刷新
    // 绘制新字符
    for (;;) {
        int c = *s;
        if (c == 0) {
            break;
        }
        if (c != ' ') {
            if ('a' <= c && c <= 'h') {
                /* a~h字符使用专门的字体绘制 */
                char *q = winbuf + y * 336 + x; // vram写入地址
                char *p = charset + 16 * (c - 'a'); // 字体地址
                // 绘制8x16像素字符
                for (i = 0; i < 16; i++) {
                    if ((p[i] & 0x80) != 0) { q[0] = color; }
                    if ((p[i] & 0x40) != 0) { q[1] = color; }
                    if ((p[i] & 0x20) != 0) { q[2] = color; }
                    if ((p[i] & 0x10) != 0) { q[3] = color; }
                    if ((p[i] & 0x08) != 0) { q[4] = color; }
                    if ((p[i] & 0x04) != 0) { q[5] = color; }
                    if ((p[i] & 0x02) != 0) { q[6] = color; }
                    if ((p[i] & 0x01) != 0) { q[7] = color; }
                    q += 336; // vram跳转到下一行, 准备绘制下一行
                }
                q -= 336 * 16; // vram跳回第一行, 准备绘制下一个字符
            } else {
                /* 其他字符使用操作系统api绘制 */
                char t[2];
                t[0] = *s;
                t[1] = 0;
                api_putstrwin(win + 1, x, y, color, 1, t); // 优化性能, win+1表示不刷新图层, 最后一块刷新
            }
        }
        // 下一个字符
        s++;
        x += 8;
    }
    api_refreshwin(win, x0, y, x, y + 16);
    return;
}

/*
    获取飞机动作指令
    - i: 若不为0, 则等待i*0.01s后返回飞机动作指令; 若为0, 则等待Enter键后返回飞机动作指令
    - timer: 定时器
    - keyflag: 飞机的动作指令
*/
void wait(int i, int timer, char *keyflag) {
    // 设定退出函数的条件
    if (i > 0) {
        // 定时器退出
        api_settimer(timer, i);
        i = 128; /* 定时器输出值 */
    } else {
        // Enter键退出
        i = 0x0a; /* Enter */
    }
    // 获取飞机动作指令
    for (;;) {
        int j = api_getkey(1);
        if (j == i) {
            break;
        }
        if (j == '4') {
            keyflag[0] = 1; // left
        }
        if (j == '6') {
            keyflag[1] = 1; // right
        }
        if (j == ' ') {
            keyflag[2] = 1; // space
        }
    }
    return;
}
