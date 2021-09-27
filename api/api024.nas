[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api024.nas"]                    ; 源文件名
        GLOBAL _api_fsize              ; 函数名
; 代码段
[SECTION .text]                     
; 获取文件大小(edx:24,eax:文件缓冲区地址,ecx:文件大小获取模式(0:普通文件大小,1:当前读取位置到文件开头起算的偏移量,2:当前读取位置到文件末尾起算的偏移量),eax(返回值):文件大小)
_api_fsize:             ; int api_fsize(int fhandle, int mode);
        MOV             EDX,24
        MOV             EAX,[ESP+4]
        MOV             ECX,[ESP+8]
        INT             0x40
        RET
