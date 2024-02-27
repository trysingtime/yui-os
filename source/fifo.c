/*
    FIFO值      中断类型
    0~1         光标闪烁定时器
    3           3秒定时器
    10          10秒定时器
    256~511     键盘输入(键盘控制器读入的值再加上256)
    512~767     鼠标输入(键盘控制器读入的值再加上512)
*/

#include "bootpack.h"

#define FLAGS_OVERRUN       0x0001

/*
    初始化缓冲区
    - fifo: 缓冲区结构体(地址)
    - size: 缓冲区总大小
    - buf: 缓冲区地址
    - task: 满足条件后自动唤醒的task, task=0: 禁止自动唤醒
*/
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task) {
    fifo->buf = buf; // 缓冲区地址
    fifo->size = size; // 总大小
    fifo->free = size; // 空余大小
    fifo->flags = 0; // 溢出标识
    fifo->p = 0; // 写入位置
    fifo->q = 0; // 读出位置
    fifo->task = task;
    return;
}

/*
    缓冲区写入数据
*/
int fifo32_put(struct FIFO32 *fifo, int data) {
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
    /* 如果FIFO设置了task, 且task当前不是正在运行, 则唤醒task */
    if (fifo->task != 0 && fifo->task->flags != 2) {
        task_run(fifo->task, -1, 0); // level为-1, 不改变task层级; priority为0, 不改变task优先级
    }
    return 0;
}

/*
    缓冲区读出数据
*/
int fifo32_get(struct FIFO32 *fifo) {
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
int fifo32_status(struct FIFO32 *fifo) {
    return fifo->size - fifo->free;
}
