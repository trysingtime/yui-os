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
        GLOBAL _asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c, _asm_inthandler0d
        GLOBAL _memtest_sub
        GLOBAL _farjmp, _farcall, _start_app
        GLOBAL _asm_system_api
	EXTERN _inthandler20, _inthandler21, _inthandler27, _inthandler2c, _inthandler0d
        EXTERN _system_api

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
        PUSHAD                          ; 调用函数前,通用寄存器全部入栈
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
        IRETD                           ; 使用IDT中断触发操作系统(段号2)的_asm_console_putchar函数, 因此操作系统上要相应使用IRETD回应

; 调用C语言inthandler21方法(来自PS/2键盘的中断)
_asm_inthandler21:
        PUSH	ES
        PUSH	DS
        PUSHAD                          ; 调用函数前,通用寄存器全部入栈
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
        PUSHAD                          ; 调用函数前,通用寄存器全部入栈
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
        PUSHAD                          ; 调用函数前,通用寄存器全部入栈
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

; 调用C语言inthandler0d方法(异常中断), 在x86架构规范中, 当应用程序试图破坏操作系统或者违背操作系统设置时自动产生0x0d中断
_asm_inthandler0d:
        STI
        PUSH	ES
        PUSH	DS
        PUSHAD                          ; 调用函数前,通用寄存器全部入栈
        MOV	EAX,ESP                 
        PUSH	EAX
        MOV	AX,SS                   ; 调用C语言函数前, SS,DS,ES设置成相同(C语言规范)
        MOV	DS,AX                   
        MOV	ES,AX
        CALL	_inthandler0d
        CMP     EAX,0
        JNE     end_app                 ; 函数返回值不为0, 则结束app, 此时EAX中保存着tss.esp0的地址        
        POP	EAX
        POPAD
        POP	DS
        POP	ES
        ADD     ESP,4                   ; INT 0x0d中需要这句
        IRETD                           ; 使用IDT中断触发操作系统(段号2)的_asm_console_putchar函数, 因此操作系统上要相应使用IRETD回应


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


; 启动app, 跳转app前设定段寄存器(ES/SS/DS/FS/GS), app代码地址cs:eip, app数据地址(栈地址)ds:esp
_start_app:             ; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
                PUSHAD                          ; 32位通用寄存器的值全部入栈(入栈顺序EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX)
                MOV             EAX,[ESP+36]    ; 参数eip, 一般为0
                MOV             ECX,[ESP+40]    ; 参数cs, 为app代码段段号
                MOV             EDX,[ESP+44]    ; 参数esp, 此参数为栈大小, 也即栈顶
                MOV             EBX,[ESP+48]    ; 参数ds/ss, 为app数据段段号
                MOV             EBP,[ESP+52]    ; 参数tss_esp0, 应用程序专用段需要在TSS中注册操作系统的段号和ESP(将操作系统的ESP和段号先后压入TSS栈esp0)
                MOV             [EBP],ESP       ; 将操作系统的ESP压入TSS.esp0
                MOV             [EBP+4],SS      ; 将操作系统的段号压入TSS.esp0
; 操作系统跨段调用app前设定段寄存器(因为app注册了应用专用段, 因此无需手动设定和切换栈)
                MOV             ES,BX
                MOV             DS,BX
                MOV             FS,BX
                MOV             GS,BX
; 运行app, 应用程序专用段不允许操作系统far-CALL/far-JMP应用程序, 因此通过RETF(far-CALL的回应,本质是从栈中将地址POP,然后far-JMP)来实现
                OR              ECX,3           ; 将app代码段号和3进行OR运算, 使用RETF调用app的小技巧
                OR              EBX,3           ; 将app数据段号和3进行OR运算, 使用RETF调用app的小技巧
                PUSH            EBX             ; 参数ds/ss, 为app数据段段号
                PUSH            EDX             ; 参数esp, 此参数为栈大小, 也即栈顶
                PUSH            ECX             ; 参数cs, 为app代码段段号
                PUSH            EAX             ; 参数eip, 一般为0
                RETF                            ; 从栈中将地址POP,然后far-JMP(cs:eip, ss:esp),从而实现app调用


; 系统API中断函数, 由INT 0x40触发, 根据ebx值调用系统函数
_asm_system_api:        ; int system_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
; 中断触发后CPU会自动禁止中断
                STI                             ; 中断触发后CPU会自动禁止中断, 此处开启中断
                PUSH            DS
                PUSH            ES
                PUSHAD                          ; 调用函数前,通用寄存器全部入栈(入栈顺序EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX)
; 调用系统API前设定段寄存器(因为app注册了应用专用段, 因此无需手动设定和切换栈)
                PUSHAD                          ; 函数所需的参数入栈, 此时的栈是app的栈, 需要将这些入栈的参数转移到操作系统栈, 因注册了应用专用段, 无需手动设定和切换栈
                MOV             AX,SS           ; 将操作系统段号存入段寄存器
                MOV             DS,AX
                MOV             ES,AX
                CALL            _system_api     ; 调用C语言函数system_api, 根据ebx值来判断调用哪个函数
                CMP             EAX,0
                JNE             end_app         ; 函数返回值不为0, 则结束app, 此时EAX中保存着tss.esp0的地址
                ADD             ESP,32          ; 丢弃之前入栈的参数
                POPAD                           ; 还原通用寄存器
                POP             ES
                POP             DS
                IRETD                           ; 使用IDT中断触发操作系统(段号2)的_asm_console_putchar函数, 因此操作系统上要相应使用IRETD回应

; 结束app
end_app:
                MOV             ESP,[EAX]       ; tss.esp0的地址, 该地址在start_app()时将操作系统的ESP和段号入栈, 此时还原(ss:esp), 使指令回到cmd_app(), 从而结束app
                POPAD
                RET                             ; 因start_app()是通过RETF调用app的, 此处RET将直接返回cmd_app()
