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
void io_stihlt(void); // 允许中断并待机
int io_in8(int port); // 从指定端口读取一个字节
void io_out8(int port, int data); //向指定设备(port)输出数据
int io_load_eflags(void); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
void io_store_eflags(int eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
char read_mem8(int addr); // 从addr指定的地址读取一个字节
void load_gdtr(int limit, int addr); // 把已知的GDT起始地址和段个数加载到GDTR寄存器
void load_idtr(int limit, int addr); // 把已知的IDT起始地址和中断个数加载到IDTR寄存器
int load_cr0(void); // CR0寄存器(32位),bit30+bit29置1禁止缓存,bit31置为0禁用分页,bit0置为1切换到保护模式
void store_cr0(int cr0);
void asm_inthandler21(void); // 键盘中断处理函数
void asm_inthandler27(void); // 电气噪声处理函数
void asm_inthandler2c(void); // 鼠标中断处理函数
unsigned int memtest_sub(unsigned int start, unsigned int end); // 内存容量检查

/* fifo.c */

// 缓冲区结构
struct FIFO8 {
    unsigned char *buf; // 缓存区地址
    int p, q, size, free, flags; // 写入位置, 读出位置, 缓存区总大小, 空余大小, 溢出标识 
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf); // 初始化缓冲区
int fifo8_put(struct FIFO8 *fifo, unsigned char data); // 缓冲区写入1字节
int fifo8_get(struct FIFO8 *fifo); // 缓冲区读出1字节
int fifo8_status(struct FIFO8 *fifo); // 缓冲区当前深度

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
#define ADR_IDT			0x0026f800 // IDT起始地址
#define LIMIT_IDT		0x000007ff // IDT上限地址
#define ADR_GDT			0x00270000 // GDT起始地址
#define LIMIT_GDT		0x0000ffff // GDT上限地址
#define ADR_BOTPAK		0x00280000 // 段号2起始地址
#define LIMIT_BOTPAK	0x0007ffff // 段号上限地址
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e

/* int.c */

void init_pic(void); // 初始化PIC
void inthandler2c(int *esp); // 电气噪声处理函数
#define PIC0_IMR		0x0021  // IMR(interrupt mask register)地址: PIC的8位寄存器
/* 
    ICW(initial control word): 有4个(ICW1-ICW4)
    - ICW1和ICW4配置与PIC主板的配线方式, 根据硬件已固定
    - ICW3(8位)每位置为1对应一个从PIC, 根据硬件已固定
    - ICW2(8位)决定IRQ触发时哪一个中断信号(例如INT 0x20), CPU根据IDT设置调用中断处理函数(需自己配置IDT和编写该处理函数)
*/ 
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_IMR		0x00a1  // IMR(interrupt mask register)地址: PIC1的8位寄存器
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */

void inthandler21(int *esp); // 键盘中断处理函数
void wait_KBC_sendready(void);
void init_keyboard(void);
extern struct FIFO8 keyfifo;
#define PORT_KEYDAT             0x0060      /* 数据端口(键盘/鼠标/A20GATE信号线) */
#define PORT_KEYCMD             0x0064      /* 键盘控制器端口(用于设置) */

/* mouse.c */

struct MOUSE_DEC {
    unsigned char buf[3], phase; // 缓冲鼠标数据, 鼠标阶段
    int x, y, btn; // 鼠标x轴, y轴, 按键
};
void inthandler27(int *esp); // 鼠标中断处理函数
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);
extern struct FIFO8 mousefifo;

/* memory.c */

#define MEMMNG_ADDR     0x003c0000; // 内存管理表起始地址
#define MEMMNG_SIZE     4096 // 空闲内存信息总数: 使用4096个FREEINFO结构记录空闲内存信息
/*
    内存空闲信息
    使用8字节记录某一段空闲内存地址起点和大小
*/
struct FREEINFO {
    unsigned int addr, size;
};
/*
    内存空闲信息-汇总
*/
struct MEMMNG {
    int rows;        // 内存空闲信息条数
    int maxrows;     // row最大值
    int lostsize;   // 内存空闲信息条数溢出, 导致内存释放失败的内存大小总和
    int lostrows;    // 内存空闲信息条数溢出, 导致内存释放失败次数
    struct FREEINFO freeinfo[MEMMNG_SIZE]; // 内存空闲信息, 使用4096个FREEINFO结构记录空闲内存
};
unsigned int memtest(unsigned int start, unsigned int end);
void memmng_init(struct MEMMNG  *mng);
unsigned int free_memory_total(struct MEMMNG *mng);
unsigned int memory_alloc(struct MEMMNG *mng, unsigned int size);
int memory_free(struct MEMMNG *mng, unsigned int addr, unsigned int size);
unsigned int memory_alloc_4k(struct MEMMNG *mng, unsigned int size);
int memory_free_4k(struct MEMMNG *mng, unsigned int addr, unsigned int size);

/* layer.c */

#define MAX_LAYERS      256 // 最大图层数
/*
    图层
    buf: 图层关联的内容地址;
    bxsize, bysize: 图层大小;
    vx0, v0: 图层坐标
    col_inv: color(颜色)和invisible(透明度)
    height: 图层高度
    flags: 图层已使用标识
*/
struct LAYER {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct LAYERCTL *ctl;
};
/*
    图层管理
    vram, xsiez, ysize: vram地址和画面大小, 不用每次去获取BOOTINFO中的启动信息
    top: 最顶层图层高度
    layersorted: 图层根据高度升序排序索引
    layer: 图层
*/
struct LAYERCTL {
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct LAYER *layersorted[MAX_LAYERS];
    struct LAYER layers[MAX_LAYERS];
};
struct LAYERCTL *layerctl_init(struct MEMMNG *memmng, unsigned char *vram, int xsize, int ysize);
struct LAYER *layer_alloc(struct LAYERCTL * ctl);
void layer_init(struct LAYER *layer, unsigned char *buf, int xsize, int ysize, int col_inv);
void layer_refresh(struct LAYER *layer, int bx0, int by0, int bx1, int by1);
void layer_refresh_abs(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void layer_refresh_map(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void layer_updown( struct LAYER *layer, int height);
void layer_slide(struct LAYER *layer, int vx0, int vy0);
void layer_free(struct LAYER *layer);
