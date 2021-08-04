/*
    定时器由专门的硬件PIT(Programmable Interval Timer)管理,
    通过设定PIT让定时器每隔多少秒产生一次中断, 
    PIT连接IRQ0
*/

#include "bootpack.h"

#define PIT_CTRL    0x0043 // PIT管理端口
#define PIT_CNT0    0x0040 // PIT设定端口

struct TIMERCTL timerctl; // 计时管理

#define TIMER_FLAGS_INACTIVE    0 //定时器标志: 未启用
#define TIMER_FLAGS_ALLOC       1 //定时器标志: 已启用
#define TIMER_FLAGS_USING       2 //定时器标志: 运行中

/*
    初始化PIT
    进入PIT设定模式, 设置中断周期
    中断频率=主频/设定值, 定时器8254芯片主频为1193180Hz, 因此设定值设为11932(0x2e9c), 中断频率大约为100Hz, 也即10ms一次中断
*/
void init_pit(void) {
    io_out8(PIT_CTRL, 0x34); // 进入PIT设定模式
    io_out8(PIT_CNT0, 0x9c); // 输入中断周期低8位
    io_out8(PIT_CNT0, 0x2e); // 输入中断周期高8位

    // 初始化所有定时器
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timer[i].flags = TIMER_FLAGS_INACTIVE; // 初始定时器标志为0, 不启用
    }

    // 初始化哨兵
    /* 
        定义一个兜底的定时器, 确保永远会有一个定时器在队列, 但永远不会(或者非常难以)触发 
        这样的话, 新增一个定时器需要的分支从4种(唯一的定时器, 最早触发的定时器, 中间触发的定时器, 最后触发的定时器)
        变成2种(最早触发的定时器, 中间触发的定时器)
    */
    struct TIMER *t;
    t = timer_alloc();
    t->next = 0; // 哨兵就是最后一个定时器
    t->timeout = 0xffffffff; // 将下一时刻最大化(保证不会被触发)
    t->flags = TIMER_FLAGS_USING;

    // 初始化定时控制器
    timerctl.count = 0; // 初始计时0
    timerctl.nextnode = t; // 初始化下一节点为哨兵
    timerctl.nexttime = t->timeout; // 初始化下一触发时刻为哨兵时刻
    return;
}

/*
    初始化定时器
    fifo, data: 定时器触发后往fifo缓冲区发送数据data
*/
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

/*
    获取一个未启用的定时器
    遍历所有定时器, 返回状态未未启用的一个定时器
*/
struct TIMER *timer_alloc(void) {
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == TIMER_FLAGS_INACTIVE) {
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;
}

/*
    释放定时器
    将指定定时器的状态改为未启用
*/
void timer_free(struct TIMER *timer) {
    timer->flags = TIMER_FLAGS_INACTIVE;
    return;
}

/*
    设置定时器
    - timer: 指定的定时器
    - timeout: 倒计时, 秒数为timeout/100
*/
void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timeout + timerctl.count; // 这里使用正计时来实现倒计时效果
    timer->flags = TIMER_FLAGS_USING;
    // 注册索引前关闭中断
    int e;
    e = io_load_eflags();
    io_cli();
    // 注册定时器到定时控制器
    /*由于哨兵的存在, 新增定时器只有两种分支(最早触发的定时器, 中间触发的定时器)*/
    struct TIMER *t, *s;
    t = timerctl.nextnode;
    /* 新增的此定时器最早触发 */
    if (timer->timeout <= t->timeout) {
        // 更新定时控制器
        timerctl.nextnode = timer;
        timerctl.nexttime = timer->timeout;
        // 设置定时器下一节点
        timer->next = t;
        // 还原中断
        io_store_eflags(e);
        return;
    }
    /* 新增的此定时器需要遍历寻找插入位置 */
    for (;;) {
        s = t;
        t = t->next;
        if (timer->timeout <= t->timeout) {
            /* 新增的此定时器定位于s和t之间 */
            // 更新定时控制器
            s->next = timer;
            // 设置定时器下一节点
            timer->next = t;
            // 还原中断
            io_store_eflags(e);
            return;
        }
    }
}

/* 来自定时器的中断(IRQ0, INT 0x20)*/
void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60); // 通知PIC0(IRQ01~07)/IRQ-00(定时器中断)已接收到中断, 继续监听下一个中断
    timerctl.count++;

    // 未到下一个指定时刻
    if (timerctl.nexttime > timerctl.count) {
        return;
    }

    // 已到下一个指定时刻, 根据链表遍历定时器, 判断是哪些定时器(可能同时触发多个)
    int i;
    struct TIMER *timer;
    timer = timerctl.nextnode;
    for (;;) {
        if (timer->timeout > timerctl.count) {
            // 后续定时器时刻未到, 退出循环
            break;
        }
        // 该定时器时刻已到, 往fifo缓冲区发送数据
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;
    }

    // 有i个定时器同时被触发了, 更新定时控制器信息
    timerctl.nextnode = timer;
    timerctl.nexttime = timerctl.nextnode->timeout;
    return;
}
