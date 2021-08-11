#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

/*
    返回当前正在活动的task内存地址
*/
struct TASK *task_current(void) {
    struct TASKLEVEL *tasklevel = &taskctl->level[taskctl->current_level];
    return tasklevel->index[tasklevel->current];
}

/*
    向任务层级(tasklevel)添加一个任务(task)
*/
void task_add(struct TASK *task) {
    struct TASKLEVEL *tasklevel = &taskctl->level[task->level];
    task->flags = 2; // 状态: 正在运行
    tasklevel->index[tasklevel->running_number] = task; // 任务所在层级正在运行的任务索引加一
    tasklevel->running_number++; // 任务所在层级正在运行的任务数量加1
    return;
}

/*
    向任务层级(tasklevel)移除(休眠)一个任务(task)
*/
void task_remove(struct TASK *task) {
    struct TASKLEVEL *tasklevel = &taskctl->level[task->level];
    /* 遍历所有正在运行的任务, 找出要休眠的是哪一个 */
    int i;
    for (i = 0; i < tasklevel->running_number; i++) {
        if (tasklevel->index[i] == task) {
            break;
        }
    }
    /* 如果要休眠的任务在当前任务前面, 任务休眠后需要修改当前任务编号 */
    if (i < tasklevel->current) {
        tasklevel->current--;
    }
    /* 休眠任务, 更新任务层级信息*/
    tasklevel->running_number--;
    // 当前任务序列超过上限, 进行修正
    if (tasklevel->current >= tasklevel->running_number) {
        tasklevel->current = 0;
    }
    // 将任务从正在运行任务索引中删除 
    for(; i < tasklevel->running_number; i++) {
        tasklevel->index[i] = tasklevel->index[i + 1];
    }
    /* 休眠任务 */
    task->flags = 1; // 从"正在运行"更新为"正在使用"
    return;
}

/*
    更新任务层级信息
    - 任务层级信息有变动, 需要遍历所有任务层级, 更新当前活动的层级
    - 若层级有变动则后续需要进行farjmp, 不然task就会产生偏差
*/
void task_level_update(void) {
    /* 遍历所有任务层级, 找出存在活动task的最上层level */
    int i;
    for (i = 0; i < MAX_TASK_LEVELS; i++) {
        if (taskctl->level[i].running_number > 0) {
            break;
        }
    }
    /* 更新任务层级信息 */
    taskctl->current_level = i;
    // 层级信息更新完毕, 关闭"需要更新标识"
    taskctl->level_change = 0;
    return;
}

/*
    哨兵任务
    - 永久HLT, 防止所有任务都休眠, 至少有一个任务在执行
*/
void task_idle_implement(void) {
    for (;;) {
        io_hlt();
    }
}

/*
    任务控制器初始化, 并返回当前任务
    - memmng: 内存控制器
*/
struct TASK *taskctl_init(struct MEMMNG *memmng) {
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    // 为任务控制器分配内存空间
    taskctl = (struct TASKCTL *) memory_alloc_4k(memmng, sizeof (struct TASKCTL));
    // 初始化所有任务层级(tasklevel)
    int i;
    for (i = 0; i < MAX_TASK_LEVELS; i++) {
        taskctl->level[i].running_number = 0; // 正在运行的任务数量为0
        taskctl->level[i].current = 0; // 正在运行的任务编号为0
    }
    // 初始化所有任务(TSS)
    for (i = 0; i < MAX_TASKS; i++) {
            taskctl->tasks[i].flags = 0; // 状态: 未激活
            taskctl->tasks[i].selector = (TASK_GDT0 + i) * 8; // 段号依次增加
            set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks[i].tss, AR_TSS32); // TSS段注册到GDT
    }
    // 主任务(当前流程也是任务)
    struct TASK *task;
    task = task_alloc();
    task->level = 0; // 分配任务层级为0(最高)
    task->flags = 2; // 状态: 正在运行
    task->priority = 2; // 0.02s切换间隔
    // 此任务注册到任务层级(tasklevel)中
    task_add(task);
    task_level_update(); // 更新当前活动的任务层级
    // 此任务注册到TR寄存器
    load_tr(task->selector); // TR(task register)寄存器: 让CPU记住当前运行哪一个任务, 任务切换时, TR的值会自动变化, TR寄存器规定GDT编号*8
    // 启动任务切换定时器
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority); // 以priority为任务间隔

    // 哨兵任务(防止所有任务都休眠)
    struct TASK *idle = task_alloc();
    // 初始化哨兵任务寄存器
    idle->tss.esp = memory_alloc_4k(memmng, 64 * 1024) + 64 * 1024; // 任务B使用的栈(64KB), esp存入栈顶(栈末尾高位地址)的地址
    idle->tss.eip = (int) &task_idle_implement;
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8; // 使用段号2
    idle->tss.ss = 1 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    // 启动哨兵任务
    task_run(idle, MAX_TASK_LEVELS - 1, 1); // task层级为最低, 防止所有任务都休眠时系统异常

    // 返回主任务
    return task;
}

/*
    获取一个未激活的任务
*/
struct TASK *task_alloc(void) {
    struct TASK *task;
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        if (taskctl->tasks[i].flags == 0) {
            // 升序找到未激活的任务
            task = &taskctl->tasks[i];
            task->flags = 1; // 状态从未激活转为正在使用
            // 初始化TSS
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            // 初始化TSS的寄存器
            task->tss.eflags = 0x00000202; // 中断标志IF置1, 允许中断, STI后EFLAGS的值就是这个, 此处手动设置
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            return task;
        }
    }
    return 0; // 全部任务都已使用
}

/*
    运行或更新指定任务
    - 将指定任务状态置为2(正在运行), 并更新任务控制器
    - task: 指定的任务
    - level: 任务层级, level<0不改变task层级
    - priority: 任务优先级(任务切换间隔), priority=0不改变该task优先级
*/
void task_run(struct TASK *task, int level, int priority) {
    /* 更新任务优先级, 即使任务正在运行也可以仅改变优先级 */
    if (priority > 0) {
        task->priority = priority;
    }
    /* 更新任务层级 */
    // 若level<0, 不更新
    if (level < 0) {
        level = task->level;
    }
    // 若level有变动, 则先删除原有层级, 后续设置好level后重新唤醒
    if (task->flags == 2 && task->level != level) {
        task_remove(task); // 此处执行后, flags已经不为2
    }
    // 指定task正在休眠则根据指定level唤醒, 若level有变动也需要重新唤醒
    if (task->flags != 2) {
        task->level = level;
        task_add(task);
    }
    taskctl->level_change = 1; // 任务层级已变动, 此处更新标识, 下次切换任务前需要更新层级信息
    return;
}

/*
    任务休眠
*/
void task_sleep(struct TASK *task) {
    if (task->flags == 2) {
        // 任务休眠前获取当前任务
        struct TASK *current_task = task_current();
        // 任务层级中移除本任务
        task_remove(task);
        /* 要休眠的任务是自身, 则需要自动切换到下一任务*/
        if (task == current_task) {
            // 更新层级信息
            task_level_update();
            // 更新后, 获取下一任务, 并跳转
            current_task = task_current();
            farjmp(0, current_task->selector); // farjmp到TSS段号, 因为是TSS, 识别为任务切换
        }
    }
    return;
}

/*
    切换到下一个任务
*/
void task_switch(void) {
    struct TASKLEVEL *tasklevel = &taskctl->level[taskctl->current_level];
    struct TASK *current_task = tasklevel->index[tasklevel->current];
    /* 切换任务层级 */
    if (taskctl->level_change != 0) {
        // 若任务层级有变动, 则更新层级信息
        task_level_update();
        tasklevel = &taskctl->level[taskctl->current_level];
    }

    /* 切换任务 */
    tasklevel->current++;
    if (tasklevel->current == tasklevel->running_number) {
        // 若当前任务是最后一个, 则下一个任务为第一个任务
        tasklevel->current = 0;
    }
    // 重置任务切换定时器, 根据下一task优先级再次计时
    struct TASK *next_task = tasklevel->index[tasklevel->current];
    timer_settime(task_timer, next_task->priority);
    // farjmp
    if (next_task != current_task) {
        farjmp(0, next_task->selector); // farjmp到TSS段号, 因为是TSS, 识别为任务切换
    }
    return;
}
