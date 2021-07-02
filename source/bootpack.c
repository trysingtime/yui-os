void io_hlt(void); // 待机
void io_cli(void); // 中断标志置0, 禁止中断
void io_out8(int port, int data); //向指定设备(port)输出数据
int io_load_eflags(void); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
void io_store_eflags(int eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))

void init_palette(void); // 初始化调色盘
void set_palette(int start, int end, unsigned char *rgb); // 设置调色盘
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); // 绘制矩形

// 定义色号和颜色映射关系
#define COL8_000000		0   /*  0:黑 */
#define COL8_FF0000		1   /*  1:梁红 */
#define COL8_00FF00		2   /*  2:亮绿 */
#define COL8_FFFF00		3   /*  3:亮黄 */
#define COL8_0000FF		4   /*  4:亮蓝 */
#define COL8_FF00FF		5   /*  5:亮紫 */
#define COL8_00FFFF		6   /*  6:浅亮蓝 */
#define COL8_FFFFFF		7   /*  7:白 */
#define COL8_C6C6C6		8   /*  8:亮灰 */
#define COL8_840000		9   /*  9:暗红 */
#define COL8_008400		10  /* 10:暗绿 */
#define COL8_848400		11  /* 11:暗黄 */
#define COL8_000084		12  /* 12:暗青 */
#define COL8_840084		13  /* 13:暗紫 */
#define COL8_008484		14  /* 14:浅暗蓝 */
#define COL8_848484		15  /* 15:暗灰 */

void HariMain(void) {
    char *vram; // VRAM起始地址
    int xsize, ysize; // 分辨率

    init_palette(); // 设定调色盘
    vram = (char *) 0xa0000;
    xsize = 320;
    ysize = 200;
    
    // 绘制多个矩形
	boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29); // 桌面背景色-浅暗蓝
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28); // 过渡-灰白
	boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27); // 过渡-白
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1); // 任务栏背景色-亮灰

	boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24); // 开始按钮上边框-白
	boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4); // 开始按钮左边框-白
	boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4); // 开始按钮底边阴影-暗灰
	boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5); // 开始按钮右边阴影-暗灰
	boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3); // 开始按钮底边框-黑
	boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3); // 开始按钮右边框-黑

	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24); // 任务状态栏凹槽上边框-暗灰
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4); // 任务状态栏凹槽左边框-暗灰
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3); // 任务状态栏凹槽底边框-白
	boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3); // 任务状态栏凹槽右边框-白

    // 待机
    for (;;) {
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}

void init_palette() {
    static unsigned char table_rgb[16 * 3] = {
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
    set_palette(0, 15, table_rgb);
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
    *vram: vram起始地址
    xsize: 分辨率x轴大小
    color: 色号
    x0, y0, x1, y1: 矩形位置
*/
void boxfill8(unsigned char *vram, int xsize, unsigned char color, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x<= x1; x++) {
            // 像素坐标 = vram(0xa0000) + x + y * xsize(320)
            vram[x + y * xsize] = color;
        }
    }
    return;
}
