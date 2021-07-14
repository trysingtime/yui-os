# include "bootpack.h"

/*
    PIC(programmable interrupt controller)初始化
    - PIC两个(PIC0/PIC1), 每个8个端口(IRQ0-IRQ7), PIC0_IRQ2连接PIC1的输出
*/
void init_pic(void) {
    // IMR(interrupt mask register): PIC的8位寄存器, 对应8个端口, 对应位值为1则屏蔽该端口
    io_out8(PIC0_IMR,  0xff  ); /* 禁止所有中断 */
	io_out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

    /* 
        ICW(initial control word): 有4个(ICW1-ICW4)
        - ICW1和ICW4配置与PIC主板的配线方式, 根据硬件已固定
        - ICW3(8位)每位置为1对应一个从PIC, 根据硬件已固定
        - ICW2(8位)决定IRQ触发时哪一个中断信号(例如INT 0x20), CPU根据IDT设置调用中断处理函数(需自己配置IDT和编写该处理函数)
    */ 
	io_out8(PIC0_ICW1, 0x11  ); /* 边缘触发模式（edge trigger mode） */
	io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7由INT20-27接收(INT 0x00~0x0f被CPU使用) */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1由IRQ2相连 */
	io_out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC1_ICW1, 0x11  ); /* 边缘触发模式（edge trigger mode） */
	io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15由INT28-2f接收 */
	io_out8(PIC1_ICW3, 2     ); /* PIC1由IRQ2连接 */
	io_out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外全部禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 禁止所有中断 */

	return;
}

/* 来自PS/2键盘的中断(IRQ1, INT 0x21)*/
void inthandler21(int *esp) {
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->screenx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->screenx, 0, 0, COL8_FFFFFF, "INT 21 (IRQ-1) : PS/2 keyboard");
	for (;;) {
		io_hlt();
	}
}

/* 来自PS/2鼠标的中断(IRQ12, INT 0x2c) */
void inthandler2c(int *esp) {
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->screenx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->screenx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
	for (;;) {
		io_hlt();
	}
}

/* PIC0中断的不完整策略 */
/* 这个中断在Athlon64X2上通过芯片组提供的便利，只需执行一次 */
/* 这个中断只是接收，不执行任何操作 */
/* 为什么不处理？
	→  因为这个中断可能是电气噪声引发的、只是处理一些重要的情况。*/
void inthandler27(int *esp) {
	io_out8(PIC0_OCW2, 0x67); /* 通知PIC的IRQ-07 */
	return;
}
