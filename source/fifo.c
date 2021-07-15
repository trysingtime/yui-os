#include "bootpack.h"

#define FLAGS_OVERRUN       0x0001

/*
    初始化缓冲区
*/
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
    fifo->buf = buf; // 缓冲区地址
    fifo->size = size; // 总大小
    fifo->free = size; // 空余大小
    fifo->flags = 0; // 溢出标识
    fifo->p = 0; // 写入位置
    fifo->q = 0; // 读出位置
    return;
}

/*
    缓冲区写入数据(1字节)
*/
int fifo8_put(struct FIFO8 *fifo, unsigned char data) {
    if (fifo->free == 0) {
        // 缓冲区已满, 无法放入数据, 设置溢出标志, 并返回-1
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo -> p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

/*
    缓冲区读出数据(1字节)
*/
int fifo8_get(struct FIFO8 *fifo) {
    int data;
    if (fifo->free == fifo->size) {
        return -1; // 缓冲区为空, 没有数据可以读
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/*
    缓冲区当前深度
*/
int fifo8_status(struct FIFO8 *fifo) {
    return fifo->size - fifo->free;
}
