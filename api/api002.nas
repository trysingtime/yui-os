[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api002.nas"]                    ; 源文件名
        GLOBAL _api_putstr0         ; 函数名
; 代码段
[SECTION .text]                     
; 显示字符串(以0结尾)
_api_putstr0:           ; void api_putstr0(char *s);
        PUSH            EBX                     
        MOV             EDX,2                   ; 根据EDX值判断调用的系统函数, 2为console_putstr0(), 显示字符串(以0结尾)
        MOV             EBX,[ESP+8]             ; 该API需要一个参数: 要打印的字符串, 通过堆栈获取, 并放入EBX
        INT             0x40                    ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        POP             EBX
        RET                                     ; 此处无需far-RET, 因为操作系统上对编译后的(.hrb)文件做了修改, 会在此RET后在其上级函数上far-RET
        