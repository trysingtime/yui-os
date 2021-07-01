; naskfunc
; TAB=4

[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "naskfunc.nas"]       ; 源文件名
        GLOBAL _io_hlt      ; 函数名(前面需加_)

; 以下是实际的函数
[SECTION .text]             ; 目标文件中写了这些之后再写程序

_io_hlt:    ; void io_hlt(void);
    HLT
    RET