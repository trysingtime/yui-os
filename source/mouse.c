#include "bootpack.h"

struct FIFO8 mousefifo;
/* 来自PS/2鼠标的中断(IRQ12, INT 0x2c) */
void inthandler2c(int *esp) {
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64); // 通知PIC1(IRQ08~15)/IRQ-12(鼠标中断)已接收到中断, 继续监听下一个中断
	io_out8(PIC0_OCW2, 0x62); // 通知PIC0(IRQ01~07)/IRQ-02(从PIC中断)已接收到中断, 继续监听下一个中断
	data = io_in8(PORT_KEYDAT); // 从端口0x0060(鼠标)读取一个字节
	fifo8_put(&mousefifo, data); // 将data写入缓冲区
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
    }
    if (mdec->phase == 1) {
        // 鼠标第一字节
        if ((data & 0xc8) == 0x08) {
            // 鼠标第一字节必须符合(0x00xx1xxx)格式, 第4,5位(低位起)与移动相关, 第0~2位(低位起)与点击相关
            // 该检查用于防止异常情况, 鼠标输入3个字节不连续导致错位, 此处忽略错误的输入
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
        // 鼠标第二字节(x轴)
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
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