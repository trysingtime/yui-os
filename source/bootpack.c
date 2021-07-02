void io_hlt(void); // 待机
void io_cli(void); // 中断标志置0, 禁止中断
void io_out8(int port, int data); //向指定设备(port)输出数据
int io_load_eflags(void); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
void io_store_eflags(int eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))

void init_palette(void); // 初始化s调色盘
void set_palette(int start, int end, unsigned char *rgb); // 设置调色盘

void HariMain(void) {
    int i;
    char *p;

    init_palette(); // 设定调色盘

    p = (char *) 0xa0000; // VRAM起始地址
    for (i = 0; i <= 0xffff; i++) {
        // 等价write_mem8(i, i & 0x0f);
        p[i] = i & 0x0f;
    }

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
