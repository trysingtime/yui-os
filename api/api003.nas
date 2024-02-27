[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api003.nas"]                    ; 源文件名
        GLOBAL _api_putstr1         ; 函数名
; 代码段
[SECTION .text]                     
; 显示字符串(指定长度)
_api_putstr1:           ; void api_putstr1(char *s, int l);
        PUSH            EBX                     
        MOV             EDX,3                   ; 根据EDX值判断调用的系统函数, 3为console_putstr1(), 显示字符串(指定s长度)
        MOV             EBX,[ESP+8]             ; s
        MOV             ECX,[ESP+12]            ; l
        INT             0x40                    ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        POP             EBX
        RET                                     ; 此处无需far-RET, 因为操作系统上对编译后的(.hrb)文件做了修改, 会在此RET后在其上级函数上far-RET
     