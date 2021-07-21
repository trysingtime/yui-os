#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
    unsigned char buf[3], phase; // 缓冲鼠标数据, 鼠标阶段
    int x, y, btn; // 鼠标x轴, y轴, 按键
};

extern struct FIFO8 keyfifo, mousefifo;
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    struct MOUSE_DEC mdec;

    init_gdtidt(); // 初始化GDT/IDT
    init_pic(); // 初始化PIC
    io_sti(); // 允许中断

    fifo8_init(&keyfifo, 32, keybuf); // 初始化键盘缓冲区
    fifo8_init(&mousefifo, 128, mousebuf); // 初始化鼠标缓冲区
    // 开放PIC, 键盘是IRQ1, PIC1是IRQ2, 鼠标是IRQ12
	io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

    init_keyboard(); // 初始化键盘控制电路(包含鼠标控制电路)

    init_palette(); // 设定调色盘
    init_screen8(bootinfo -> vram, bootinfo -> screenx, bootinfo -> screeny); // 初始化屏幕
    // 绘制鼠标指针
    mx = (bootinfo -> screenx - 16) / 2; // 计算屏幕中间点(减去指针本身)
    my = (bootinfo -> screeny - 28 - 16) / 2; // 计算屏幕中间点(减去任务栏和指针本身)
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(bootinfo -> vram, bootinfo -> screenx, 16, 16, mx, my, mcursor, 16);
    // 绘制鼠标坐标
    sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 0, COL8_FFFFFF, s);
    // 绘制字符串
 	// putfonts8_asc(bootinfo->vram, bootinfo->screenx,  8,  8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 31, 31, COL8_000000, "Haribote OS."); // 文字阴影效果
	putfonts8_asc(bootinfo->vram, bootinfo->screenx, 30, 30, COL8_FFFFFF, "Haribote OS.");

    enable_mouse(&mdec); // 启用鼠标本身

    for (;;) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt(); // 区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断
        } else {
            if (fifo8_status(&keyfifo) != 0 ) {
                i = fifo8_get(&keyfifo);
                io_sti();

                boxfill8(bootinfo->vram, bootinfo->screenx, COL8_008484, 0, 16, 15, 31);
                sprintf(s, "%02X", i);
                putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();

                if (mouse_decode(&mdec, i) != 0) {
                    // 鼠标3字节已完整, 显示鼠标数值
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    // mdec.btn第0位: 左键, 第1位: 右键, 第2位:中建
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(bootinfo->vram, bootinfo->screenx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(bootinfo->vram, bootinfo->screenx, 32, 16, COL8_FFFFFF, s);

                    // 显示鼠标指针移动
                    boxfill8(bootinfo -> vram, bootinfo -> screenx, COL8_008484, mx, my, mx + 15, my + 15); // 隐藏鼠标(绘制背景色矩形遮住之前绘制好的鼠标)
                    boxfill8(bootinfo -> vram, bootinfo -> screenx, COL8_008484, 0, 0, 79, 15); // 隐藏坐标(绘制背景色矩形遮住之前绘制好的坐标)
                    // 计算鼠标x, y轴的数值, 基于屏幕中心点
                    mx += mdec.x;
                    my += mdec.y;
                    // 防止鼠标超出屏幕
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > bootinfo->screenx - 16) {
                        mx = bootinfo->screenx - 16;
                    }
                    if (my > bootinfo->screeny - 16) {
                        my = bootinfo->screeny - 16;
                    }
                    // 绘制
                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc(bootinfo->vram, bootinfo->screenx, 0, 0, COL8_FFFFFF, s); // 显示坐标
                    putblock8_8(bootinfo -> vram, bootinfo -> screenx, 16, 16, mx, my, mcursor, 16); // 显示鼠标
                }
            }
        }
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}

#define PORT_KEYDAT             0x0060      /* 键盘端口 */
#define PORT_KEYSTA             0x0064      /* 键盘控制器端口(用于读取) */
#define PORT_KEYCMD             0x0064      /* 键盘控制器端口(用于设置) */
#define KEYSTA_SEND_NOTREADY	0x02        /* 键盘控制器数据倒数第二位为0表示键盘控制器准备好接收控制指令 */
#define KEYCMD_WRITE_MODE		0x60        /* 键盘控制器-模式设定模式 */
#define KBC_MODE				0x47        /* 键盘-鼠标模式 */

/*
    等待键盘控制电路准备完毕
    键盘控制电路慢于CPU电路, 因此键盘准备好后再通知CPU, 通过设备号码0x0064读取的数据倒数第二位为0判断
*/
void wait_KBC_sendready(void) {
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            return;
        }
    }
}

/*
    初始化键盘控制电路(同时也初始化了鼠标控制电路)
*/
void init_keyboard(void) {
    wait_KBC_sendready(); // 等待键盘控制电路准备完毕
    // 使键盘控制电路(0x0064)进入模式设定模式(0x60)
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    // 设置数据端口(键盘/鼠标/A20GATE信号线)(0x0060)模式(0x47), 启用键盘控制电路, 同时也启用了鼠标控制电路启用
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4

/*
    启用鼠标本身
    启用鼠标需要使启用鼠标控制电路和鼠标本身, 鼠标控制电路包含于键盘控制电路中, 
        因此启用鼠标需先初始化键盘控制电路, 再使鼠标本身启用, 此处实现后者
*/
void enable_mouse(struct MOUSE_DEC *mdec) {
    wait_KBC_sendready(); // 等待键盘控制电路准备完毕
    // 使键盘控制电路(0x0064)进入鼠标控制电路模式(0xd4), 下一个数据会自动发送给鼠标
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    // 设置数据端口(键盘/鼠标/A20GATE信号线)(0x0060)模式(0xf4), 启用鼠标, 且鼠标会马上产生一个中断, 生成0xfa的消息
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0; // 将鼠标阶段置0(初始状态)
    return;
}

/*
    解码鼠标输入
*/
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data) {
    if (mdec->phase == 0) {
        if  (data == 0xfa) {
            // 鼠标启用后, 自动生成一个中断, 生成0xfa的信号, 此时鼠标阶段由0切1
            mdec->phase = 1;
        }
        return 0;
    } else if (mdec->phase == 1) {
        // 鼠标第一字节
        if ((data & 0xc8) == 0x08) {
            // 鼠标第一字节必须符合(0x00xx1xxx)格式, 第4,5位(低位起)与移动相关, 第0~2位(低位起)与点击相关
            // 该检查用于防止异常情况, 鼠标输入3个字节不连续导致错位, 此处忽略错误的输入
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    } else if (mdec->phase == 2) {
        // 鼠标第二字节(x轴)
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    } else if (mdec->phase == 3) {
        // 鼠标第三字节(y轴)
        mdec->buf[2] = data;
        mdec->phase = 1;

        mdec->btn = mdec->buf[0] & 0x07; // 鼠标第一字节第0~2位(低位起)与点击相关(第0位: 左键, 第1位: 右键, 第2位:中建)
        mdec->x = mdec->buf[1]; // x轴
        mdec->y = mdec->buf[2]; // y轴

        // 根据鼠标第一字节第4,5位(低位起)与移动相关, 用于设置x,y轴的高24位
        // 若值为0, 则高24位置0, 若第4位值为1, 则x轴高24位置1, 若第5位值为1, 则y轴高24位置1
        if ((mdec->buf[0] & 0x10) != 0) {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }

        mdec->y = - mdec->y; // 鼠标y方向与画面符号相反
        return 1;
    }
    return -1;
}
