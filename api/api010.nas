[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api010.nas"]                    ; 源文件名
        GLOBAL _api_free            ; 函数名
; 代码段
[SECTION .text]                     
; 释放指定起始地址和大小的内存(edx:10,ebx:内存控制器地址,eax:释放的内存空间起始地址,ecx:释放的内存空间字节数)
_api_free:              ; void api_free(char *addr, int size)
        PUSH            EBX
        MOV             EDX,10
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             EAX,[ESP+8]             ; 释放的内存空间起始地址(addr)   
        MOV             ECX,[ESP+12]            ; 释放的内存空间字节数(size)
        INT             0x40
        POP             EBX
        RET
