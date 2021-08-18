#include "bootpack.h"
#include <stdio.h> // sprintf()

/*
    任务B
*/
void task_b_implement(struct LAYER *layer) {
    struct FIFO32 fifo;
    struct TIMER *timer_ls;
    int i, fifobuf[128], count = 0, count0 = 0;
    char s[12];

    // 设置中断缓冲区
    fifo32_init(&fifo, 128, fifobuf, 0);
    // 设置定时器
    timer_ls = timer_alloc(); // 运行速度测试定时器
    timer_init(timer_ls, &fifo, 100);
    timer_settime(timer_ls, 100); // 1s

    // 处理中断
    for (;;) {
        // 计数
        count++;
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            //io_stihlt();
            io_sti(); // 性能测试时使用, 高速计数器需要全力运行, 因此取消io_hlt();
        } else {
            // 定时器中断处理
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 100) {
                sprintf(s, "%11d", count - count0);
                putfonts8_asc_layer(layer, 24, 28, COL8_000000, COL8_C6C6C6, s, 11);
                count0 = count;
                timer_settime(timer_ls, 100); // 重置计时器, 再次计时
            }
        }
    }
}

