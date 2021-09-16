; 攻击其他app数据段内存, 从而破坏app
[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

[FILE "crack7.nas"]
        GLOBAL  _HariMain

[SECTION .text]
_HariMain:
        MOV     AX,1005*8
        MOV     DS,AX
        CMP     DWORD [DS:0x0004],'Hari'    ; 获取段号1005的0x0004判断是不是app的代码段, 如果不是则结束, 是则攻击该app

        JNE     fin

        MOV     ECX,[DS:0x0000]             ; 通过app代码段0x0000获取app数据段大小
        MOV     AX,2005*8                   
        MOV     DS,AX                       ; 设定DS为app数据段, 这样可以操作其他的app数据

crackloop:                                  ; 遍历app数据段, 将所有数据置为123
        ADD     ECX,-1
        MOV     BYTE [DS:ECX],123
        CMP     ECX,0
        JNE     crackloop

fin:
        MOV     EDX,4
        INT     0x40
