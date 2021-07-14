; naskfunc
; TAB=4

[FORMAT "WCOFF"]            ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "naskfunc.nas"]                   ; 源文件名
        ; 函数名
        GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt
        GLOBAL _io_in8, _io_in16, _io_in32
        GLOBAL _io_out8, _io_out16, _io_out32
        GLOBAL _io_load_eflags, _io_store_eflags
        GLOBAL _write_mem8, _read_mem8
        GLOBAL _load_gdtr, _load_idtr
        GLOBAL _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
	EXTERN _inthandler21, _inthandler27, _inthandler2c

[SECTION .text]             ; 目标文件中写了这些之后再写程序

; 以下是实际的函数

; CPU待机
_io_hlt:        ; void io_hlt(void);
        HLT
        RET

; 中断许可标志置0
_io_cli:        ; void io_cli(void);
        CLI
        RET

; 中断许可标志置1
_io_sti:        ; void io_sti(void);
        STI
        RET

; 中断许可标志置1并待机
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

; 读取EFLAGS寄存器(32位)(包含进位标志(第0位),中断标志(第9位))
_io_load_eflags:        ; int io_load_eflags(void);
        PUSHFD          ; 直接操作EFLAGS, push flags double-world
        POP     EAX
        RET

; 还原EFLAGS寄存器(32位)(包含进位标志(第0位),中断标志(第9位))
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

; 调用C语言inthandler21方法
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

; 调用C语言inthandler27方法
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

; 调用C语言inthandler2c方法
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
