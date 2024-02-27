[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api022.nas"]                    ; 源文件名
        GLOBAL _api_fclose             ; 函数名
; 代码段
[SECTION .text]                     
; 关闭文件(edx:22,eax:文件缓冲区地址)
_api_fclose:              ; void api_fclose(int fhandle);
        MOV             EDX,22
        MOV             EAX,[ESP+4]
        INT             0x40
        RET
