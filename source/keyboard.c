#include "bootpack.h"

struct FIFO8 keyfifo;
/* 来自PS/2键盘的中断(IRQ1, INT 0x21)*/
void inthandler21(int *esp) {
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61); // 通知PIC0(IRQ01~07)/IRQ-01(键盘中断)已接收到中断, 继续监听下一个中断
	data = io_in8(PORT_KEYDAT); // 从端口0x0060(键盘)读取一个字节
	fifo8_put(&keyfifo, data); // 将data写入缓冲区
	return;
}

#define PORT_KEYSTA             0x0064      /* 键盘控制器端口(用于读取) */
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