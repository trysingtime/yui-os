[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_end         ; 函数名
; 代码段
[SECTION .text]                     
; 强制结束app
_api_end:               ; void api_end(void);
        MOV             EDX,4
        INT             0x40                    ; 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用end_app结束程序直接返回到cmd_app()
