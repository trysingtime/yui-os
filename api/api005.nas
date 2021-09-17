[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_openwin         ; 函数名
; 代码段
[SECTION .text]                     
; 显示窗口(edx:5,ebx:窗口内容地址,esi:窗口宽度,edi:窗口高度,eax:窗口颜色和透明度,ecx:窗口标题,返回值放入eax)
_api_openwin:           ; int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBX
        MOV             EDX,5
        MOV             EBX,[ESP+16]            ; buf
        MOV             ESI,[ESP+20]            ; xsize
        MOV             EDI,[ESP+24]            ; ysize
        MOV             EAX,[ESP+28]            ; col_inv
        MOV             ECX,[ESP+32]            ; title
        INT             0x40
        POP             EBX
        POP             ESI
        POP             EDI
        RET
