; haribote-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; 加载bootpack
DSKCAC	EQU		0x00100000		; 磁盘缓存的位置
DSKCAC0	EQU		0x00008000		; 磁盘缓存的位置（实模式）

; 指定一段内存地址, 用以缓存BOOT_INFO
CYLS	EQU		0x0ff0			; 启动时读取的柱面数
LEDS	EQU		0x0ff1			; 启动时键盘LED的状态
VMODE	EQU		0x0ff2			; 显卡模式为多少位彩色
SCREENX	EQU		0x0ff4			; 画面分辨率X
SCREENY	EQU		0x0ff6			; 画面分辨率Y
VRAM	EQU		0x0ff8			; 图像缓冲区开始地址

        ORG     0xc200

; 当前CPU为16位模式, 后续要切换到32位模式, 但BIOS是16位机器语言写的, 切换后无法再调用BIOS功能, 因此切换前把需调用BIOS功能的提前完成
; 通过BIOS设置显卡模式(video mode)
        MOV     AL,0x13             ; 0x03:16色字符模式,80x25;0x12:VGA图形模式, 640X480X4位彩色; 0x13:VGA图形模式,320x200x8位彩色; 0x6a: 扩展VGA图形模式, 800x600x4位彩色
        MOV     AH,0x00
        INT     0x10

; 保存BOOT_INFO
        MOV     BYTE [VMODE],8      ; 几位色
        MOV     WORD [SCREENX],320    ; 分辨率X
        MOV     WORD [SCREENY],200    ; 分辨率Y
        MOV     DWORD [VRAM],0x000a0000     ; 图像缓冲区开始地址(不同显卡模式对应不同VRAM地址, 0x13对应0xa0000~0xaffff)

; 通过BIOS取得键盘各种LED指示灯的状态
        MOV     AH,0x02
        INT     0x16
        MOV     [LEDS],AL

; PIC关闭一切中断, 根据AT兼容机的规范, PIC初始化需在CLI前进行
		MOV		AL,0xff
		OUT		0x21,AL			; 禁止PIC0中断
		NOP						; 如果连续指定OUT指令, 有些机种无法正常运行
		OUT		0xa1,AL			; 禁止PIC1中断
		CLI						; 禁止CPU级别的中断

; 让CPU支持1M以上内存, 设置A20GATE
; 类似于鼠标, 通过键盘控制电路激活A20GATE信号线
		CALL	waitkbdout		; 等待键盘控制电路准备完毕
		MOV		AL,0xd1
		OUT		0x64,AL			; 使键盘控制电路(0x0064)进入(0xd1)模式
		CALL	waitkbdout
		MOV		AL,0xdf
		OUT		0x60,AL			; 设置数据端口(键盘/鼠标/A20GATE信号线)(0x0060)模式(0xfd), 使A20GATE(0x0060)激活
		CALL	waitkbdout		; 再次检查键盘控制电路处于准备状态, 确保A20GATE切实处理完毕

; 切换到保护模式(protected virtual address mode)
; 区别实模式(real address mode), 计算内存地址时, 实模式使用段寄存器的值直接指定地址值的一部分(段寄存器*16+指定地址)
; 保护模式则通过GDT使用段寄存器的值指定并非实际存在的地址号码(段号起始地址+指定地址), 小结: 有无使用GDT
[INSTRSET "i486p"]				; 说明使用486指令(32位)
		LGDT	[GDTR0]			; 设置临时GDT
		MOV		EAX,CR0			; control register 0, 非常重要的寄存器
		AND		EAX,0x7fffffff	; bit31置为0（禁用分页）
		OR		EAX,0x00000001	; bit0置为1 (切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush	; CPU为了加快指令执行速度, 使用pipeline(管道)机制提前解释好了后续指令, 切换模式后需要重新解释, 因此使用JMP指令改变后续
pipelineflush:
		MOV		AX,1*8			;  进入保护模式后, 除了CS(防止混乱后续处理)以外所有段寄存器值从0x0000变成0x0008, 相当于gdt+1段
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; 内存复制,参考README中的内存分布图
; bootpack地址开始的512KB(大于实际bootpack.hrb的大小)内容复制到0x00280000地址
		MOV		ESI,bootpack	; 转送源地址
		MOV		EDI,BOTPAK		; 转送目的地址(0x00280000)
		MOV		ECX,512*1024/4	; 转送数据大小(单位是双字节, 需要实际字节除于4)
		CALL	memcpy
; 启动区(0x7c00~0x7dff, 512字节)(启动区的内容是自动读取磁盘C0H0S1而来)复制到0x00100000地址
		MOV		ESI,0x7c00		; 启动区(0x7c00~0x7dff)
		MOV		EDI,DSKCAC		; 0x00100000
		MOV		ECX,512/4
		CALL	memcpy
; 磁盘剩余数据(C0H0S2开始的18柱面/2磁头/18扇区, C0H0S1的启动区已经在上面复制了)被读取到0x8200开始的地址, 现在复制到0x00100200, 现在0x00100000~0x00267fff就是完整磁盘数据
		MOV		ESI,DSKCAC0+512	; 0x8200
		MOV		EDI,DSKCAC+512	; 0x00100200
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 除以4得到字节数
		SUB		ECX,512/4		; C0H0S1的启动区已经在上面复制了, 忽略启动区数据, 因此减去512字节
		CALL	memcpy

; 必须由asmhead来完成的工作, 至此全部完毕, 以后就交由bootpack来完成
; 堆栈初始化及启动bootpack
		MOV		EBX,BOTPAK		; BOTPAK值为0x00280000, 是bootpack.hrb起始地址
		MOV		ECX,[EBX+16]	; 转送数据大小, [EBX+16]值为0x11a8(可以打开bootpack.hbr以十六进制查看)
		ADD		ECX,3			; ECX += 3
		SHR		ECX,2			; ECX /= 4
		JZ		skip			; 若[EBX+16]值小于1, 跳过复制内存
		MOV		ESI,[EBX+20]	; 转送源地址, [EBX+20]值为0x10c8
		ADD		ESI,EBX			; 此时ESI值为0x002810c8, 表示bootpack.hrb第0x11a8字节
		MOV		EDI,[EBX+12]	; 转送目的地址, [EBX+12]值为0x00310000, 该内存用于堆栈
		CALL	memcpy			; 将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000地址
skip:
		MOV		ESP,[EBX+12]	; 堆栈的初始化(0x00300000~0x003fffff用于堆栈)
		JMP		DWORD 2*8:0x0000001b	; 跳转到段号2(0x280000(bootpack.hrb))的0x1b地址, 即0x28001b(bootpack.hrb的0x1b地址), 也即启动bootpack

; 以下为功能函数, 用于被上述方法调用

; 等待键盘控制电路准备完毕
; 键盘控制电路慢于CPU电路, 因此键盘准备好后再通知CPU, 通过设备号码0x0064读取的数据倒数第二位为0判断
waitkbdout:
		IN		AL,0x64			; 键盘控制器设备号0x64
		AND		AL,0x02			; 通过设备号码0x0064读取的数据倒数第二位为0判断
		IN		AL,0x60			; 数据端口(键盘/鼠标/A20GATE信号线), 这里顺便把数据读取出来
		JNZ		waitkbdout		; 一直读取,直至为空
		RET

; 内存复制函数(单位双字节)
memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 每双字节循环一次
		RET

; 设定临时GDT
; GDT结构32位, 依次为(short limit_low, base_low;char base_mid, access_right;char limit_high, base_high;)
; LGDT寄存器48位, 前16位为GDT段个数(段个数*8), 后32位为GDT起始地址
		ALIGNB	16				; 一直添加DBO, 直到地址能被16整除(如果GDT0的地址不能被8整数除, 向段寄存器复制的MOV指令就会慢一些)
GDT0:
		RESB	8							; GDT0是一种特定的GDT, 0是空区域(null sector), 不能在此定义段
		DW		0xffff,0x0000,0x9200,0x00cf	; 临时定义段号1(可以读写的段),起始地址0x00000000,上限地址0xffffffff(页模式为1因此需额外前补3个fff),段属性0x4092(0xc092)
		DW		0xffff,0x0000,0x9a28,0x0047	; 临时定义段号2(可以执行的段)(bootpack用),起始地址0x00280000,上限地址0x7ffff,段属性0x409a
		DW		0
GDTR0:
		DW		8*3-1						; GDT段个数(段个数*每段8字节), 此处段个数为3
		DD		GDT0						; GDT起始地址

		ALIGNB	16
bootpack: