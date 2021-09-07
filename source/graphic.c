#include "bootpack.h"

/*
    初始化调色盘
*/
void init_palette(void) {
    // 设定标准16色(0~15)
    static unsigned char color_16[16 * 3] = {
        0x00, 0x00, 0x00,	/*  0:黑 */
		0xff, 0x00, 0x00,	/*  1:梁红 */
		0x00, 0xff, 0x00,	/*  2:亮绿 */
		0xff, 0xff, 0x00,	/*  3:亮黄 */
		0x00, 0x00, 0xff,	/*  4:亮蓝 */
		0xff, 0x00, 0xff,	/*  5:亮紫 */
		0x00, 0xff, 0xff,	/*  6:浅亮蓝 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:亮灰 */
		0x84, 0x00, 0x00,	/*  9:暗红 */
		0x00, 0x84, 0x00,	/* 10:暗绿 */
		0x84, 0x84, 0x00,	/* 11:暗黄 */
		0x00, 0x00, 0x84,	/* 12:暗青 */
		0x84, 0x00, 0x84,	/* 13:暗紫 */
		0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
		0x84, 0x84, 0x84	/* 15:暗灰 */
    };
    set_palette(0, 15, color_16);

    // 设定rgb色(16~231)
    static unsigned char color_rgb[6 * 6 * 6 * 3];
    int r, g, b;
    for (b = 0; b < 6; b++) {
        for (g = 0; g < 6; g++) {
            for (r = 0; r < 6; r++) {
                color_rgb[(r + g * 6 + b * 36) * 3 + 0] = r * 51; // 256色分为6个亮度等级(包括0), 每个51
                color_rgb[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
                color_rgb[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, color_rgb);
    return;
}

/*
    调色板访问步骤:
    1. 屏蔽中断(例如CLI)
    2. 设定调色板: 将调色板号写入0x03c8, 之后按照R,G,B顺序写入0x03c9三次
    3. 读取调色板: 将调色板号写入0x03c7, 之后按照R,G,B顺序读取0x03c9三次
    4. 恢复中断(例如STI)
*/
void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags(); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
    io_cli(); // 中断标志置0, 禁止中断

    io_out8(0x03c8, start); // 调色板号写入0x03c8
    for (i = start; i <= end; i++) {
        // 按照R,G,B顺序写入0x03c9三次
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
    return;
}

/*
    绘制矩形
    vram: vram起始地址
    screenx: 分辨率x轴大小
    color: 色号
    x0, y0, x1, y1: 矩形位置
*/
void boxfill8(unsigned char *vram, int screenx, unsigned char color, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x<= x1; x++) {
            // 像素坐标 = vram(0xa0000) + x + y * xsize(320)
            vram[x + y * screenx] = color;
        }
    }
    return;
}

/*
    初始化桌面
    vram: vram起始地址
    screenx: 分辨率x轴大小
    screeny: 分辨率y轴大小
*/
void init_screen8(char *vram, int screenx, int screeny) {
    // 绘制多个矩形
	boxfill8(vram, screenx, COL8_008484,  0,                    0, screenx -  1, screeny - 29); // 桌面背景色-浅暗蓝
	boxfill8(vram, screenx, COL8_C6C6C6,  0,         screeny - 28, screenx -  1, screeny - 28); // 过渡-灰白
	boxfill8(vram, screenx, COL8_FFFFFF,  0,         screeny - 27, screenx -  1, screeny - 27); // 过渡-白
	boxfill8(vram, screenx, COL8_C6C6C6,  0,         screeny - 26, screenx -  1, screeny -  1); // 任务栏背景色-亮灰

	boxfill8(vram, screenx, COL8_FFFFFF,  3,         screeny - 24, 59,         screeny - 24); // 开始按钮上边框-白
	boxfill8(vram, screenx, COL8_FFFFFF,  2,         screeny - 24,  2,         screeny -  4); // 开始按钮左边框-白
	boxfill8(vram, screenx, COL8_848484,  3,         screeny -  4, 59,         screeny -  4); // 开始按钮底边阴影-暗灰
	boxfill8(vram, screenx, COL8_848484, 59,         screeny - 23, 59,         screeny -  5); // 开始按钮右边阴影-暗灰
	boxfill8(vram, screenx, COL8_000000,  2,         screeny -  3, 59,         screeny -  3); // 开始按钮底边框-黑
	boxfill8(vram, screenx, COL8_000000, 60,         screeny - 24, 60,         screeny -  3); // 开始按钮右边框-黑

	boxfill8(vram, screenx, COL8_848484, screenx - 47, screeny - 24, screenx -  4, screeny - 24); // 任务状态栏凹槽上边框-暗灰
	boxfill8(vram, screenx, COL8_848484, screenx - 47, screeny - 23, screenx - 47, screeny -  4); // 任务状态栏凹槽左边框-暗灰
	boxfill8(vram, screenx, COL8_FFFFFF, screenx - 47, screeny -  3, screenx -  4, screeny -  3); // 任务状态栏凹槽底边框-白
	boxfill8(vram, screenx, COL8_FFFFFF, screenx -  3, screeny - 24, screenx -  3, screeny -  3); // 任务状态栏凹槽右边框-白
}

/*
    绘制字符
    vram: vram起始地址
    screenx: 分辨率x轴大小
    x, y: 字符位置
    color: 色号
    font: 字体数据(使用16字节定义一个8x16像素的字符)
*/
void putfont8(char *vram, int screenx, int x, int y, char color, char *font) {
    int i;
    char *p;
    for (i = 0; i <= 16; i++) {
        // 8x16像素字符, 每一行起始vram地址
        p = vram + (y + i) * screenx + x;
        if ((font[i] & 0x80) != 0) { p[0] = color; }
        if ((font[i] & 0x40) != 0) { p[1] = color; }
		if ((font[i] & 0x20) != 0) { p[2] = color; }
		if ((font[i] & 0x10) != 0) { p[3] = color; }
		if ((font[i] & 0x08) != 0) { p[4] = color; }
		if ((font[i] & 0x04) != 0) { p[5] = color; }
		if ((font[i] & 0x02) != 0) { p[6] = color; }
		if ((font[i] & 0x01) != 0) { p[7] = color; }
    }
}

/*
    绘制字符串
    vram: vram起始地址
    screenx: 分辨率x轴大小
    x, y: 字符串位置
    color: 色号
    str: 字符串
*/
void putfonts8_asc(char *vram, int screenx, int x, int y, char color, unsigned char *str) {
    extern char hankaku[4096]; // 使用16字节定义一个8x16像素的字符, 此处是4096个字符集合
    // c语言中, 字符串以0x00结尾
    for (; *str != 0x00; str++) {
        putfont8(vram, screenx, x, y, color, hankaku + *str * 16);
        x += 8;
    }
    return;
}

/*
    初始化鼠标指针(16x16像素)像素点颜色数据
    mouse: 鼠标指针像素点颜色数据存储地址
    bc: 背景色
*/
void init_mouse_cursor8(char *mouse, char bc) {
    // 定义一个16x16像素的鼠标指针
    static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
    int x, y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x ++) {
            // 边框
            if (cursor[y][x] == '*') {
                mouse[y * 16 + x] = COL8_000000;
            }
            // 字体
            if (cursor[y][x] == 'O') {
                mouse[y * 16 + x] = COL8_FFFFFF;
            }
            // 背景
            if (cursor[y][x] == '.') {
                mouse[y * 16 + x] = bc;
            }
        }
    }
}

/*
    绘制图形
    vram: vram起始地址
    screenx: 分辨率x轴大小
    pxsize, pysize: 图形大小
    px0, py0: 图形位置
    buf: 图形像素点颜色数据
    bxsize: 图形每一行像素数
*/
void putblock8_8(char *vram, int screenx, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * screenx + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
