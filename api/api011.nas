[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_point           ; 函数名
; 代码段
[SECTION .text]                     
; 在窗口中画点(edx:11,ebx:窗口图层地址,esi:x坐标,edi:y坐标,eax:颜色)
_api_point:             ; void api_point(int win, int x, int y, int col);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBX
        MOV             EDX,11
        MOV             EBX,[ESP+16]            ; win
        MOV             ESI,[ESP+20]            ; x
        MOV             EDI,[ESP+24]            ; y
        MOV             EAX,[ESP+28]            ; col
        INT             0x40
        POP             EBX
        POP             ESI
        POP             EDI
        RET
