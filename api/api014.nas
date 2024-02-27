[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api014.nas"]                    ; 源文件名
        GLOBAL _api_closewin        ; 函数名
; 代码段
[SECTION .text]                     
; 关闭窗口图层(edx:14,ebx:窗口图层地址)
_api_closewin:          ; void api_closewin(int win);
        PUSH            EBX
        MOV             EDX,14
        MOV             EBX,[ESP+8]
        INT             0x40
        POP             EBX
        RET
