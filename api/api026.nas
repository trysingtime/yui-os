[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api026.nas"]                    ; 源文件名
        GLOBAL _api_cmdline            ; 函数名
; 代码段
[SECTION .text]                     
; 获取控制台当前命令行指令(edx:26,eax:命令行缓冲区地址,ecx:最大存放字节数,eax(返回值):实际存放字节数)
_api_cmdline:             ; int api_cmdline(char *buf, int maxsize);
        PUSH            EBX
        MOV             EDX,26
        MOV             EBX,[ESP+8]
        MOV             ECX,[ESP+12]
        INT             0x40
        POP             EBX
        RET
