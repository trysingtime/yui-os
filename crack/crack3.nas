; 设定定时器中断周期, 使其中断周期最大, 从而影响系统使用
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式
    MOV     AL,0x34
    OUT     0x43,AL
    MOV     AL,0xff
    OUT     0x40,AL
    MOV     AL,0xff
    OUT     0x40,AL

; 中断频率=主频/设定值, 定时器8254芯片主频为1193180Hz, 因此设定值设为11932(0x2e9c), 中断频率大约为100Hz, 也即10ms一次中断
; 上述代码相当于:
; io_out8(PIT_CTRL, 0x34); // 进入PIT设定模式
; io_out8(PIT_CNT0, 0x9c); // 输入中断周期低8位
; io_out8(PIT_CNT0, 0x2e); // 输入中断周期高8位
