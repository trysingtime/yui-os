[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api012.nas"]                    ; 源文件名
        GLOBAL _api_refreshwin      ; 函数名
; 代码段
[SECTION .text]                     
; 窗口图层刷新(edx:12,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1)
_api_refreshwin:        ; void api_refreshwin(int win, int x0, int y0, int x1, int y1);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBX
        MOV             EDX,12
        MOV             EBX,[ESP+16]            ; win
        MOV             EAX,[ESP+20]            ; x0
        MOV             ECX,[ESP+24]            ; y0
        MOV             ESI,[ESP+28]            ; x1
        MOV             EDI,[ESP+32]            ; y1
        INT             0x40
        POP             EBX
        POP             ESI
        POP             EDI
        RET
