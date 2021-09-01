[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                   ; 源文件名
        ; 函数名
        GLOBAL _api_putchar, _api_putstr0
        GLOBAL _api_openwin, _api_refreshwin, _api_closewin
        GLOBAL _api_putstrwin, _api_boxfillwin, _api_point, _api_linewin
        GLOBAL _api_initmalloc, _api_malloc, _api_free
        GLOBAL _api_end

[SECTION .text]             ; 目标文件中写了这些之后再写程序

; 显示单个字符
_api_putchar:           ; void api_putchar(int c);
        MOV             EDX,1                   ; 根据EDX值判断调用的系统函数, 1为console_putchar(), 显示单个字符
        MOV             AL,[ESP+4]              ; 该API需要一个参数: 要打印的字符, 通过堆栈获取, 并放入EAX
        INT             0x40                    ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        RET                                     ; 此处无需far-RET, 因为操作系统上对编译后的(.hrb)文件做了修改, 会在此RET后在其上级函数上far-RET

; 显示字符串(以0结尾)
_api_putstr0:           ; void api_putstr0(char *s);
        PUSH            EBX                     
        MOV             EDX,2                   ; 根据EDX值判断调用的系统函数, 2为console_putstr0(), 显示字符串(以0结尾)
        MOV             EBX,[ESP+8]             ; 该API需要一个参数: 要打印的字符串, 通过堆栈获取, 并放入EBX
        INT             0x40                    ; 使用IDT中断触发操作系统(段号2)的_asm_system_api函数, 因此操作系统上要相应使用IRETD回应
        POP             EBX
        RET                                     ; 此处无需far-RET, 因为操作系统上对编译后的(.hrb)文件做了修改, 会在此RET后在其上级函数上far-RET

; 强制结束app
_api_end:               ; void api_end(void);
        MOV             EDX,4
        INT             0x40                    ; 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用end_app结束程序直接返回到cmd_app()

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

; 窗口显示方块(edx:7,ebx:窗口内容地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色)
_api_boxfillwin:        ; void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
        PUSH            EDI
        PUSH            ESI
        PUSH            EBP
        PUSH            EBX
        MOV             EDX,7
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

; 初始化app内存控制器(edx:8,ebx:内存控制器地址,eax:管理的内存空间起始地址,ecx:管理的内存空间字节数)
_api_initmalloc:        ; void api_initmalloc(void)
        PUSH            EBX
        MOV             EDX,8
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址(数据段相对地址)=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             EAX,EBX     
        ADD             EAX,32*1024             ; 管理的内存空间起始地址, 内存控制器占用了开头的32k, 因此需要偏移32k  
        MOV             ECX,[CS:0x0000]         ; hrb文件的0x0000存放app数据段大小(app代码段的0x0000)
        SUB             ECX,EAX                 ; 管理的内存空间字节数=app数据段大小-内存控制器地址(数据段相对地址)-内存控制器大小32k
        INT             0x40
        POP             EBX
        RET

; 分配指定大小的内存(edx:9,ebx:内存控制器地址,eax:分配的内存空间起始地址,ecx:分配的内存空间字节数, 返回值放入eax)
_api_malloc:            ; char *api_malloc(int size)
        PUSH            EBX
        MOV             EDX,9
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             ECX,[ESP+8]             ; 分配的内存空间字节数(size)
        INT             0x40
        POP             EBX
        RET

; 释放指定起始地址和大小的内存(edx:10,ebx:内存控制器地址,eax:释放的内存空间起始地址,ecx:释放的内存空间字节数)
_api_free:              ; void api_free(char *addr, int size)
        PUSH            EBX
        MOV             EDX,10
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             EAX,[ESP+8]             ; 释放的内存空间起始地址(addr)   
        MOV             ECX,[ESP+12]            ; 释放的内存空间字节数(size)
        INT             0x40
        POP             EBX
        RET

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

; 关闭窗口图层(edx:14,ebx:窗口图层地址)
_api_closewin:          ; void api_closewin(int win);
        PUSH            EBX
        MOV             EDX,14
        MOV             EBX,[ESP+8]
        INT             0x40
        POP             EBX
        RET
