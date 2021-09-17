[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_malloc          ; 函数名
; 代码段
[SECTION .text]                     
; 分配指定大小的内存(edx:9,ebx:内存控制器地址,eax:分配的内存空间起始地址,ecx:分配的内存空间字节数, 返回值放入eax)
_api_malloc:            ; char *api_malloc(int size)
        PUSH            EBX
        MOV             EDX,9
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             ECX,[ESP+8]             ; 分配的内存空间字节数(size)
        INT             0x40
        POP             EBX
        RET
