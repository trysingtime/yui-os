; 操作系统执行app:      操作系统将app注册到GDT, 例如段号1003, 然后通过far-Call执行app, app通过far-RET返回
; app调用操作系统API:   操作系统将API注册到IDT, 例如中断号0x40, app通过INT调用API, 操作系统通过IRETD返回
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式
        MOV         ECX,msg         
        MOV         EDX,1           ; 根据EDX值判断调用的系统函数, 1为console_putchar(), 显示单个字符
putloop:                            ; 逐字遍历打印
        MOV         AL,[CS:ECX]     ; 该API需要一个参数: 要打印的字符串, 放入EAX
        CMP         AL,0
        JBE         fin
        INT         0x40            ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        ADD         ECX,1
        JMP         putloop
fin:
        MOV         EDX,4
        INT         0x40            ; 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用end_app结束程序直接返回到cmd_app()
msg:
        DB          "hello",0       ; 要打印的字符