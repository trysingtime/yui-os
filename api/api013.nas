[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_linewin         ; 函数名
; 代码段
[SECTION .text]                     
; 窗口绘制直线(edx:13,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色)
_api_linewin:           ; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBP
        PUSH            EBX
        MOV             EDX,13
        MOV             EBX,[ESP+20]            ; win
        MOV             EAX,[ESP+24]            ; x0
        MOV             ECX,[ESP+28]            ; y0
        MOV             ESI,[ESP+32]            ; x1
        MOV             EDI,[ESP+36]            ; y1
        MOV             EBP,[ESP+40]            ; col
        INT             0x40
        POP             EBX
        POP             EBP
        POP             ESI
        POP             EDI
        RET
