[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "a_nask.nas"]                   ; 源文件名
        ; 函数名
        GLOBAL _api_putchar,_api_end

[SECTION .text]             ; 目标文件中写了这些之后再写程序

_api_putchar:       ; void api_putchar(int c);
        MOV         EDX,1               ; 根据EDX值判断调用的系统函数, 1为console_putchar(), 显示单个字符
        MOV         AL,[ESP+4]          ; 该API需要一个参数: 要打印的字符串, 通过堆栈获取, 并放入EAX
        INT         0x40                ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        RET                             ; 此处无需far-RET, 因为操作系统上对编译后的(.hrb)文件做了修改, 会在此RET后在其上级函数上far-RET

_api_end:           ; void api_end(void);
        MOV         EDX,4
        INT         0x40                ; 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用end_app结束程序直接返回到cmd_app()