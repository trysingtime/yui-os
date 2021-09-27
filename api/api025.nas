[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api025.nas"]                    ; 源文件名
        GLOBAL _api_fread              ; 函数名
; 代码段
[SECTION .text]                     
; 文件读取(edx:25,eax:文件缓冲区地址,ebx:读取文件目的地址,ecx:最大读取字节数,eax(返回值):本次读取到的字节数)
_api_fread:             ; int api_fread(char *buf, int maxsize, int fhandle);
        PUSH            EBX
        MOV             EDX,25
        MOV             EBX,[ESP+8]
        MOV             ECX,[ESP+12]
        MOV             EAX,[ESP+16]
        INT             0x40
        POP             EBX
        RET
