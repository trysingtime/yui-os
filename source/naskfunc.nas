; naskfunc
; TAB=4

[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "naskfunc.nas"]                   ; 源文件名
        ; 函数名
        GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt
        GLOBAL _io_in8, _io_in16, _io_in32
        GLOBAL _io_out8, _io_out16, _io_out32
        GLOBAL _io_load_eflags, _io_store_eflags
        GLOBAL _write_mem8, _read_mem8
        GLOBAL _load_gdtr, _load_idtr, _load_tr
        GLOBAL _load_cr0, _store_cr0
        GLOBAL _asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
        GLOBAL _memtest_sub
        GLOBAL _farjmp, _farcall
        GLOBAL _asm_console_putchar
	EXTERN _inthandler20, _inthandler21, _inthandler27, _inthandler2c
        EXTERN _console_putchar

[SECTION .text]             ; 目标文件中写了这些之后再写程序

; 以下是实际的函数

; CPU待机
_io_hlt:        ; void io_hlt(void);
        HLT
        RET

; 中断许可标志置0, 禁止中断
_io_cli:        ; void io_cli(void);
        CLI
        RET

; 中断许可标志置1, 允许中断
_io_sti:        ; void io_sti(void);
        STI
        RET

; 允许中断并待机(区别与"io_sti();io_hlt()", CPU规范中如果STI紧跟HLT, 那么两条指令间不受理中断)
_io_stihlt:     ; void io_stihlt(void);
        STI
        HLT
        RET

; 从指定port读取8位数据
_io_in8:        ; int io_in8(int port);
        MOV     EDX,[ESP+4]
        MOV     EAX,0
        IN      AL,DX
        RET

; 从指定port读取16位数据
_io_in16:        ; int io_in16(int port);
        MOV     EDX,[ESP+4]
        MOV     EAX,0
        IN      AX,DX
        RET

; 从指定port读取32位数据
_io_in32:        ; int io_in32(int port);
        MOV     EDX,[ESP+4]
        IN      EAX,DX
        RET

; 向指定port写入8位数据
_io_out8:       ; void io_out8(int port, int data);
        MOV     EDX,[ESP+4]
        MOV     AL,[ESP+8]
        OUT     DX,AL
        RET

; 向指定port写入16位数据
_io_out16:       ; void io_out16(int port, int data);
        MOV     EDX,[ESP+4]
        MOV     AX,[ESP+8]
        OUT     DX,AX
        RET

; 向指定port写入32位数据
_io_out32:       ; void io_out32(int port, int data);
        MOV     EDX,[ESP+4]
        MOV     EAX,[ESP+8]
        OUT     DX,EAX
        RET        

; 读取EFLAGS寄存器(32位)(包含进位标志CF(第0位),中断标志IF(第9位),AC标志位(第18位, 486CPU以上才有))
_io_load_eflags:        ; int io_load_eflags(void);
        PUSHFD          ; 直接操作EFLAGS, push flags double-world
        POP     EAX
        RET

; 还原EFLAGS寄存器(32位)(包含进位标志CF(第0位),中断标志IF(第9位),AC标志位(第18位, 486CPU以上才有))
_io_store_eflags:        ; void io_sotre_eflags(int eflags);
        MOV     EAX,[ESP+4]
        PUSH    EAX
        POPFD            ; 直接操作EFLAGS, pop flags double-world
        RET

; 将data写入addr指定的地址
_write_mem8:    ; void write_mem8(int addr, int data);
        MOV     ECX,[ESP+4]         ; 读取第一个参数addr(汇编与C联合使用, 只能使用EAX,ECX,EDX, 其他寄存器被用于C编译后的机器语言)
        MOV     AL,[ESP+8]          ; 读取第二个参数data
        MOV     [ECX],AL            ; 将data写入addr指定的地址
        RET

; 从addr指定的地址读取一个字节
_read_mem8:     ; char read_mem8(int addr);
        MOV     EDX,[ESP+4]
        MOV     AL,[EDX]
        RET

; 将GDT的段个数和起始地址保存到GDTR寄存器(48位)
_load_gdtr:	; void load_gdtr(int limit, int addr);
        MOV	AX,[ESP+4]              ; 只需要limit低位两字节(例如FFFF0000 00002700)
        MOV	[ESP+6],AX              ; 低位两字节覆盖高位两字节(例如FFFFFFFF 00002700)
        LGDT	[ESP+6]                 ; 从指定地址读取6个字节(例如FFFF002700)
        RET

; 将IDT的中断个数和起始地址保存到IDTR寄存器(48位)
_load_idtr:	; void load_idtr(int limit, int addr);
        MOV	AX,[ESP+4]		; 只需要limit低位两字节
        MOV	[ESP+6],AX              ; 低位两字节覆盖高位两字节
        LIDT	[ESP+6]                 ; 从指定地址读取6个字节
        RET

; 向TR(task register)寄存器(任务切换时值会自动变化)存入数值
_load_tr:       ; void load_tr(int tr);
        LTR     [ESP+4]
        RET

; CR0寄存器(32位),bit30+bit29置1禁止缓存,bit31置为0禁用分页,bit0置为1切换到保护模式
_load_cr0:      ; int load_cr0(void);
        MOV     EAX,CR0
        RET

_store_cr0:     ; void store_cr0(int cr0);
        MOV     EAX,[ESP+4]
        MOV     CR0,EAX
        RET

; 调用C语言inthandler20方法(定时器中断)
_asm_inthandler20:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV	EAX,ESP                 
        PUSH	EAX
        MOV	AX,SS                   ; 调用C语言函数前, SS,DS,ES设置成相同(C语言规范)
        MOV	DS,AX                   
        MOV	ES,AX
        CALL	_inthandler20
        POP	EAX
        POPAD
        POP	DS
        POP	ES
        IRETD

; 调用C语言inthandler21方法(来自PS/2键盘的中断)
_asm_inthandler21:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV	EAX,ESP                 
        PUSH	EAX
        MOV	AX,SS                   ; 调用C语言函数前, SS,DS,ES设置成相同(C语言规范)
        MOV	DS,AX                   
        MOV	ES,AX
        CALL	_inthandler21
        POP	EAX
        POPAD
        POP	DS
        POP	ES
        IRETD

; 调用C语言inthandler27方法(电气噪声中断)
_asm_inthandler27:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV	EAX,ESP
        PUSH	EAX
        MOV	AX,SS
        MOV	DS,AX
        MOV	ES,AX
        CALL	_inthandler27
        POP	EAX
        POPAD
        POP	DS
        POP	ES
        IRETD

; 调用C语言inthandler2c方法(来自PS/2鼠标的中断)
_asm_inthandler2c:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV	EAX,ESP
        PUSH	EAX
        MOV	AX,SS
        MOV	DS,AX
        MOV	ES,AX
        CALL	_inthandler2c
        POP	EAX
        POPAD
        POP	DS
        POP	ES
        IRETD

; 内存容量检查(参考C语言方法memtest_sub_c()说明)
_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; （EBX, ESI, EDI も使いたいので）
		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55			; pat0 = 0xaa55aa55;
		MOV		EDI,0x55aa55aa			; pat1 = 0x55aa55aa;
		MOV		EAX,[ESP+12+4]			; i = start;
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0xffc				; p = i + 0xffc;
		MOV		EDX,[EBX]				; old = *p;
		MOV		[EBX],ESI				; *p = pat0;
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		EDI,[EBX]				; if (*p != pat1) goto fin;
		JNE		mts_fin
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		ESI,[EBX]				; if (*p != pat0) goto fin;
		JNE		mts_fin
		MOV		[EBX],EDX				; *p = old;
		ADD		EAX,0x1000				; i += 0x1000;
		CMP		EAX,[ESP+12+8]			; if (i <= end) goto mts_loop;
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX				; *p = old;
		POP		EBX
		POP		ESI
		POP		EDI
		RET

; far跳转指令, 目的地址为cs:eip. 若cs(段号)为TSS, 识别为任务切换
_farjmp:        ; void farjmp(int eip, int cs);
                JMP     FAR [ESP+4]     ; far-JMP, 同时修改EIP和CS, 从而实现指令跳转. CS段寄存器低3位无效, 因此需要*8. 若cs(段号)为TSS, 识别为任务切换
                RET                     ; 若为任务切换, 返回后会继续执行代码, 需要RET

; far调用函数, 目的地址为cs:eip.调用的函数返回时需要使用far-RET
_farcall:       ; void farcall(int eip, int cs);
                CALL    FAR [ESP+4]     ; far-CALL, 同时修改EIP和CS, 从而实现函数调用. CS段寄存器低3位无效, 因此需要*8.
                RET                     ; 使用far-Call跨段调用其他段的函数(例如应用程序), 调用的函数返回时需要使用far-RET, 此处仅为普通RET

; 自制系统API, 显示字符
_asm_console_putchar:   ; void console_putchar(struct CONSOLE *console, int character, char move);
                PUSH            1               ; move参数入栈: 显示字符后光标是否后移
                AND             EAX,0xff        ; character参数, 只保留低8位, 高位全部置0
                PUSH            EAX             ; character参数入栈: 要显示的字符
                PUSH            DWORD [0x0fec]  ; 执行字符显示的控制台内存地址, 此处从0x0fec获取, 控制台初始化时, 将自身地址放入0x0fec
                CALL            _console_putchar; 调用C语言函数
                ADD             ESP,12          ; 函数执行完毕后将刚才入栈的数据丢弃
                RETF                            ; 应用使用far-Call跨段调用操作系统(段号2)API, 因此相应使用far-RET回应
