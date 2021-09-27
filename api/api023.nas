[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api023.nas"]                    ; 源文件名
        GLOBAL _api_fseek              ; 函数名
; 代码段
[SECTION .text]                     
; 文件定位(edx:23,eax:文件缓冲区地址,ecx:定位模式(0:定位起点为文件开头,1:定位起点为当前访问位置,2:定位起点为文件末尾)),ebx:定位偏移量
_api_fseek:             ; void api_fseek(int fhandle, int offset, int mode);
        PUSH            EBX
        MOV             EDX,23
        MOV             EAX,[ESP+8]
        MOV             EBX,[ESP+12]
        MOV             ECX,[ESP+16]
        INT             0x40
        POP             EBX
        RET
