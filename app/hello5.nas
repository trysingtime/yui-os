[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "hello5.nas"]         ; 源文件名
        GLOBAL      _HariMain

[SECTION .text]
_HariMain:
        MOV     EDX,2
        MOV     EBX,msg
        INT     0x40                        ; 调用api打印字符串(以0结尾)
        MOV     EDX,4
        INT     0x40                        ; 调用api强制停止app

[SECTION .data]
msg:
        DB      "hello, world", 0x0a, 0     ; 要打印的字符串("hello, world\n")
