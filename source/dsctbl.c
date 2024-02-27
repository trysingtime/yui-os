/*
什么是GDT/IDT:

GDT(global segment descriptor table): 全局段号记录表, GDT由SEGMENT_DESCRIPTOR(8字节)组成
- GDTR(48位)(global segment descriptor table register)低16位是段个数, 高32位是GDT起始地址
- GDT保存许多SEGMENT_DESCRIPTOR(8字节)
- SEGMENT_DESCRIPTOR存储段起始地址,上限地址及段属性
    - base(32位):该段起始地址
    - limit(段地址上限值): 20位, limit_high上4位被用于access_right
    - access_right(段的属性): 12位, limit_high高4位被用于access_right的高4位, 一般使用16位表示(xxxx0000xxxxxxxx)
        - 高4位被称为扩展访问器, 一般为"GD00"
            G表示Gbit(granularity粒度), 为0时表示limit单位为byte, 上限为1MB; 为1时表示limit单位为page(4KB), 上限为4BK*1MB=4G
            D表示段的模式, 0是16位模式, 用于80286CPU, 不能调用BIOS; 1是32位模式, 除80286外一般D=1
        - 低8位
            00000000(0x00): 未使用的记录表
            10010010(0x92): 系统专用, 可读写的段. 不可执行
            10011010(0x9a): 系统专用, 可执行的段. 可读不可写
            11110010(0xf2): 应用程序用, 可读写的段. 不可指定
            11111010(0xfa): 应用程序用, 可执行的段. 可读不可写

段地址(如DS:0x00ff)获取过程:
- 段寄存器(16位, 如DS, 但低3位不能使用, 因此可以存放8192个段号)存放段号
- GDTR存放GDT起始地址和有效个数, 根据段号和GDTR信息到GDT地址
- 根据GDT地址, 获取到GDT信息(主要是段地址), 里面存放着段起始地址, 上限地址, 段属性
- 段地址+0xff获得准确地址

IDT(interrupt descriptor table): 中断记录表, IDT由GATE_DESCRIPTOR(8字节)组成
- IDTR(interrupt descriptor table register)(48位)中保存IDT的起始地址和个数
- IDT保存许多GATE_DESCRIPTOR(8字节)
- GATE_DESCRIPTOR存储中断函数地址, 段号, 属性

应用程序专用段(段属性加上0x60)
- 应用程序专用段需要在TSS中注册操作系统的段号和ESP(将操作系统的ESP和段号先后压入TSS栈esp0)
- 应用程序专用段将会限制app使用IN/OUT/HLT/CLI/STI/CALL等一系列操作, 也无法修改段寄存器为操作系统的段号
- 应用程序专用段不允许操作系统far-CALL/far-JMP应用程序, 因此通过RETF(far-CALL的回应,本质是从栈中将地址POP,然后far-JMP)来实现
- 应用程序专用段允许应用程序far-CALL/far-JMP操作系统

TSS(task status segment)(32位, 26个int成员, 总计104字节): 任务状态段, 需要在GDT注册才能使用
- 在任务控制器初始化时将TSS注册到GDT(3~1002)
- CPU任务切换时, 将寄存器的值保存在TSS中, 后续切换回任务时, 再读取回去
- 如果一条JMP指令的目的地址段是TSS, 则解析为任务切换

LDT(local segment descriptor table): 局部段号记录表

*/

#include "bootpack.h"

/*
    初始化GDT, IDT
    设定GDT/IDT的起始地址和上限地址, 并初始化GDT/IDT(调用set_segmdesc/set_gatedesc), 定义每个段号对应的段信息/每个中断号对应的函数信息
*/
void init_gdtidt(void) {
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; // GDT起始地址
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) ADR_IDT; // IDT起始地址
    int i;

    // GDT全置为0
    for (i = 0; i <= LIMIT_GDT / 8; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 将GDT的段个数(段个数*每段8字节, 此处8192个段)和起始地址保存到GDTR寄存器
    load_gdtr(LIMIT_GDT, ADR_GDT);
    // 设置段号1
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW); // 此处段属性实际为0xc092, 段上限地址为0xfffff, set_segmdesc()会自动处理该转化
    // 设置段号2
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);

    // IDT全置为0
    for (i = 0; i <= LIMIT_IDT / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    // 将IDT的中断个数(段个数*每段8字节, 此处256中断)和起始地址保存到IDTR寄存器
    load_idtr(LIMIT_IDT, ADR_IDT);
    /*
        注册中断, 调用asm_inthandler函数, 段号为2
        - 中断号: 异常中断(0x00~0x1f), IRQ(0x20~0x2f)
        - 异常中断(0x00~0x1f): 0x00(除零异常), 0x06(非法指令异常), 0x0c(栈异常), 0x0d(一般保护异常)
        - IRQ中断(0x00~0x1f): 0x20(定时器), 0x21(键盘), 0x27(电气噪声), 0x2c(鼠标)
    */
    // 注册异常中断
    set_gatedesc(idt + 0x0c, (int) asm_inthandler0c, 2 * 8, AR_INTGATE32); // 栈异常
    set_gatedesc(idt + 0x0d, (int) asm_inthandler0d, 2 * 8, AR_INTGATE32); // 一般保护异常
    // 注册IRQ中断
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32); // 定时器
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32); // 键盘
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32); // 电气噪声
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32); // 鼠标
    // 注册系统API中断
    set_gatedesc(idt + 0x40, (int) asm_system_api, 2 * 8, AR_INTGATE32 + 0x60); // 加上0x60表明可供应用程序触发
    
    return;
}

/*
    定义每个段号对应的段信息(段起始地址, 上限地址, 段属性)
    - sd: SEGMENT_DESCRIPTOR(8字节), 存储段起始地址, 上限地址, 段属性
    - limit: 段上限地址
    - base: 段起始地址
    - access_right(段的属性): 此处参数32位, 实际12位, limit_high高4位被用于access_right的高4位, 一般使用16位表示(xxxx0000xxxxxxxx)
*/
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int access_right) {
    // 段上限地址超过0xfffff, 切换到Gbit(页模式, granularity: 粒度), 上限从1MB变成4GB
    if (limit > 0xfffff) {
        // access_right一般使用16位表示(xxxx0000xxxxxxxx), 此处设置高4位(扩展访问器)中第1位, 切换到页模式
        access_right |= 0x8000;
        limit /= 0x1000;
    }
    // 分段保存地址, 目的是兼容386等CPU
    sd -> access_right  = access_right & 0xff;
    sd -> base_low      = base & 0xffff;
    sd -> base_mid      = (base >> 16) & 0xff;
    sd -> base_high     = (base >> 24) & 0xff;
    sd -> limit_low     = limit & 0xffff;
    sd -> limit_high    = ((limit >> 16) & 0x0f) | ((access_right >> 8) & 0xf0); // limit_high高4位被用于access_right的高4位
    return;
}

/*
    每个中断号对应的函数信息
    gd: GATE_DESCRIPTOR(8字节), 存储中断函数地址, 段号, 属性
    offset: 中断函数地址(传入函数名即传入函数首地址)
    selector: 段号, 低3位不计算, 从第4位开始, 所以段号2对应值为2<<3或者2*8或者16
    access_right: 属性
*/
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int access_right) {
    gd -> access_right  = access_right & 0xff;
    gd -> dw_count      = (access_right >> 8 ) & 0xff;
    gd -> selector      = selector;
    gd -> offset_low    = offset & 0xffff;
    gd -> offset_high   = (offset >> 16) & 0xffff;
    return;
}
