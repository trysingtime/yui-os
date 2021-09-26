[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "alloca.nas"]                     ; 源文件名
        GLOBAL __alloca                  ; 函数名
; 代码段
[SECTION .text]                     
; 压栈EAX字节
__alloca:                                       ; void _alloca();
        ADD             EAX,-4                  ; EAX代表需要从栈中分配多少字节的内存空间, 此处为优化后的结构, 减去4相当于ESP加上4, 再加上JMP指令, 相当于RET(先JMP,然后ESP+4)
        SUB             ESP,EAX                 ; 压栈EAX个字节, 并弹栈4字节(EAX提前减去4)
        JMP             DWORD [ESP+EAX]         ; 相当于RET(JMP到压栈EAX前保存的ESP, 并且ESP也需弹栈4字节(已提前完成))
        