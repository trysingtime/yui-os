[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api027.nas"]                    ; 源文件名
        GLOBAL _api_getlang            ; 函数名
; 代码段
[SECTION .text]                     
; 获取控制台当前语言模式(edx:27,eax(返回值): 语言模式)
_api_getlang:             ; int _api_getlang(void);
        MOV             EDX,27
        INT             0x40
        RET
