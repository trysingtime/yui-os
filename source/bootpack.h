/* asmhead.nas */

// 缓存在指定位置的BOOT_INFO(asmhead.nas中)
struct BOOTINFO {
    char cyls;              // 启动时读取的柱面数
    char leds;              // 启动时键盘LED的状态
    char vmode;             // 显卡模式为多少位彩色
    char reserve;
    short screenx, screeny; // 画面分辨率
    char *vram; // VRAM起始地址
};
#define ADR_BOOTINFO    0x00000ff0

/* naskfunc.nas */

void io_hlt(void); // 待机
void io_cli(void); // 中断标志置0, 禁止中断
void io_sti(void); // 中断标志置1, 允许中断
void io_out8(int port, int data); //向指定设备(port)输出数据
int io_load_eflags(void); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
void io_store_eflags(int eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
char read_mem8(int addr); // 从addr指定的地址读取一个字节
void load_gdtr(int limit, int addr); // 把已知的GDT起始地址和段个数加载到GDTR寄存器
void load_idtr(int limit, int addr); // 把已知的IDT起始地址和中断个数加载到IDTR寄存器
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

/* graphic.c */

void init_palette(void); // 初始化调色盘
void set_palette(int start, int end, unsigned char *rgb); // 设置调色盘
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); // 绘制矩形
void init_screen8(char *vram, int screenx, int screeny); // 初始化屏幕
void putfont8(char *vram, int screenx, int x, int y, char color, char *font); // 绘制字符
void putfonts8_asc(char *vram, int screenx, int x, int y, char color, unsigned char *str); // 绘制字符串
void init_mouse_cursor8(char *mouse, char bc); // 初始化鼠标指针(16x16像素)像素点颜色数据
void putblock8_8(char *vram, int screenx, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize); // 绘制图形

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

/* dsctbl.c */

/*
SEGMENT_DESCRIPTOR(8字节, GDT单元结构)保存段起始地址,上限地址及段属性
- base(32位):该段起始地址
- limit(20位, limit_high上4位被用于access_right):该段地址上限值
- access_right(段的属性): 12位, limit_high高4位被用于access_right的高4位, 一般使用16位表示(xxxx0000xxxxxxxx)
    -- 高4位被称为扩展访问器, 一般为"GD00"
        G表示Gbit(granularity粒度), 为0时表示limit单位为byte, 上限为1MB; 为1时表示limit单位为page(4KB), 上限为4BK*1MB=4G
        D表示段的模式, 0是16位模式, 用于80286CPU, 不能调用BIOS; 1是32位模式, 除80286外一般D=1
    -- 低8位
        00000000(0x00): 未使用的记录表
        10010010(0x92): 系统专用, 可读写的段. 不可执行
        10011010(0x9a): 系统专用, 可执行的段. 可读不可写
        11110010(0xf2): 应用程序用, 可读写的段. 不可指定
        11111010(0xfa): 应用程序用, 可执行的段. 可读不可写
*/
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

/*
    GATE_DESCRIPTOR(8字节), 存储中断函数地址, 段号, 属性
    - offset: 中断函数地址(传入函数名即传入函数首地址)
    - selector: 段号
    - access_right: 属性
*/
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

void init_gdtidt(void); // 设定GDT/IDT的起始地址和上限地址, 并初始化GDT/IDT(调用set_segmdesc/set_gatedesc), 定义每个段号对应的段信息/每个中断号对应的函数信息
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int access_right); // 设置每个段号对应的段信息
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int access_right); // 设置每个中断号对应的函数信息
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e

/* int.c */

void init_pic(void); // 初始化PIC
void inthandler21(int *esp); // 键盘中断处理函数
void inthandler27(int *esp); // 鼠标中断处理函数
void inthandler2c(int *esp); // 电气噪声处理函数
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
