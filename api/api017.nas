[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_inittimer       ; 函数名
; 代码段
[SECTION .text]                     
; 设置定时器发送的数据(edx:17,ebx:定时器地址,eax:数据)
_api_inittimer:         ; void api_inittimer(int timer, int data);
        PUSH            EBX
        MOV             EDX,17
        MOV             EBX,[ESP+8]
        MOV             EAX,[ESP+12]
        INT             0x40
        POP             EBX
        RET
