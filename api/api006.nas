[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_putstrwin       ; 函数名
; 代码段
[SECTION .text]                     
; 窗口显示字符串(edx:6,ebx:窗口内容地址,esi:显示的x坐标,edi:显示的y坐标,eax:颜色,ecx:字符长度,ebp:字符串)
_api_putstrwin:         ; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBP
        PUSH            EBX
        MOV             EDX,6
        MOV             EBX,[ESP+20]            ; win
        MOV             ESI,[ESP+24]            ; x
        MOV             EDI,[ESP+28]            ; y
        MOV             EAX,[ESP+32]            ; col
        MOV             ECX,[ESP+36]            ; len
        MOV             EBP,[ESP+40]            ; str
        INT             0x40
        POP             EBX
        POP             EBP
        POP             ESI
        POP             EDI
        RET
