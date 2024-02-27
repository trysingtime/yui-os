; haribote-ipl
; TAB=4

CYLS	EQU		15				; 读取柱面数
; MBR程序是在没有操作系统支撑的情况下, BIOS用了一条指令:jmp 0:7c00来强制修改CS值=0
; 当程序从7c00h开始运行的时候，CS变化了，但是DS/ES的值并没有做任何的改变
; 在汇编程序中,凡是在使用jmp AA:BB指令(使用了段寄存器)之后，都需要重置DS/ES的值，否则就没有办法正常访问到新程序中的任何内存数据
; 机器启动后自动将ipl加载到0x7c00~0x7dff这512字节, 此处告诉编译器这个事实, 让其后续可以使用(0x7c00+相对位置)来操作内存
		ORG	0x7c00

; 标准FAT12格式软盘

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 启动区名称(字符串8字节)					; 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x49, 0x50, 0x4c
		DW		512				; 每个扇区(sector)的大小(必须为512字节)		; 0x00, 0x02
		DB		1				; 簇(cluster)的大小(必须为1扇区)			; 0x01
		DW		1				; FAT的起始位置(扇区)						; 0x01, 0x00
		DB		2				; FAT的个数(必须为2)						; 0x02
		DW		224				; 根目录的大小(一般设成224项)				; 0xe0, 0x00
		DW		2880			; 该磁盘的大小(必须为2880扇区)				; 0x40, 0x0b
		DB		0xf0			; 磁盘的种类(必须为0xf0)
		DW		9				; FAT的长度(必须为9扇区)					; 0x09, 0x00
		DW		18				; 1个磁道(track)有几个扇区(必须是18)		; 0x12, 0x00
		DW		2				; 磁头数(必须是2)							; 0x02, 0x00
		DD		0				; 不使用分区, 必须是0						; 0x00, 0x00, 0x00, 0x00
		DD		2880			; 重写一次磁盘大小							; 0x40, 0x0b, 0x00, 0x00
		DB		0, 0, 0x29		; 意义不明, 固定							; 0x00, 0x00, 0x29
		DB		0xffffffff		; (可能是)卷标号码							; 0xff, 0xff, 0xff, 0xff
		DB		"HARIBOTEOS "	; 磁盘的名称(11字节)						; 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x2d, 0x4f, 0x53, 0x20, 0x20, 0x20
		DB		"FAT12   "		; 磁盘格式名称(8字节)						; 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20
		RESB	18				; 空出18字节								; 0x00...

; 程序主体
entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX
		MOV 	SP,0x7c00
		MOV		DS,AX			; 在汇编程序中,凡是在使用jmp AA:BB指令(使用了段寄存器)之后，都需要重置DS/ES的值，否则就没有办法正常访问到新程序中的任何内存数据

; 读取磁盘
		MOV		AX,0x0820
		MOV		ES,AX			; 缓冲地址(校验及寻道时不使用), [ES:BX] = ESx16+BX(20位寻址), 此处实际内存地址为0x8200
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
		MOV		BX,18*2*CYLS-1	; 总共要读取扇区数
		CALL	readfast

; 读取完毕, 跳转到haribote.sys
		MOV		BYTE [0x0ff0],CYLS	; 将CYLS的值写到内存地址0x0ff0中, 后续haribote.sys可以获取该值
		JMP		0xc200				; haribote.sys保存在磁盘的0x004200位置, 磁盘的前512字节(0x0200)被自动读取到内存的0x7c00~0x7dff, 
									; 后续内容被上面手动读取0x8200, 因此haribote.sys读取到内存后位于0x8200+(0x4200-0x0200)=0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS
		JMP		putloop
fin:
		HLT						; 让CPU停止, 等待指令
		JMP		fin				; 无限循环
msg:
		DB		0x0a, 0x0a		; 两个换行
		DB		"load error"	; 打印信息									; 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64
		DB		0x0a			; 换行
		DB		0x00

readfast:						; 一次性读取多个扇区(ES:读取的起始地址, CH:柱面, DH:磁头, CL:扇区, BX:总共要读取的扇区数)
		; 64K对齐(以64k为单位, 读取起始地址不是64K倍数则此次读取只能读取到64K倍数, 每个扇区512字节, 则需要读取到128倍数为区号的扇区)
		MOV		AX,ES			; ES:读取的起始地址(段寄存器, 需要*16才是真正的地址)
		SHL		AX,3			; 当前扇区为ES*16/512=ES/32, 需要右移5位, 将结果存于AH中, 则再次左移8位, 合计左移3位
		AND		AH,0x7f			; 当前扇区%128, 余数表示64K对齐后在当前64K(128扇区中)第几个扇区
		MOV		AL,128
		SUB		AL,AH			; 128-当前扇区号=64K剩余要读取的扇区数

		; 64K剩余要读取的扇区数与总共要读取的扇区数, 哪个小使用哪个
		MOV		AH,BL			; if (BH != 0) { AH = 18 }
		CMP		BH,0
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH}
		JBE		.skip2
		MOV		AL,AH
.skip2:
		; 64K剩余要读取的扇区数与总共要读取的扇区数与当前柱面剩余扇区(1~19扇区), 哪个小使用哪个
		MOV		AH,19
		SUB		AH,CL			; AH = 19 - CL
		CMP		AL,AH			; if(AL > AH) { AL = AH }
		JBE		.skip3
		MOV		AL,AH
.skip3:
		PUSH	BX
		MOV		SI,0			; 记录失败次数
retry:
		MOV		AH,0x02			; 0x02:读盘;0x03:写盘;0x04:校验;0x0c:寻道;0x00:系统复位
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; 调用磁盘BIOS(返回FLACS.CF, 0:没有错误(AH置为0), 1:有错误(AH置为错误码))
		JNC		next
; 读取重试
		ADD		SI,1
		CMP		SI,5
		JAE		error			; SI>=5
		MOV		AH,0x00			; 系统复位
		MOV		DL,0x00			; A驱动器
		INT		0x13
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
; 读取18扇区
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; 将ES内容弹栈到BX, ES无法计算,通过BX实现
		; 将内存地址后移(AL*0x20), [ES:BX]=ESx16+BX, 因此相当于ES后移(AL*0x200=AL*512)
		SHR		BX,5
		MOV		AH,0
		ADD		BX,AX			; BX += AL
		SHL		BX,5
		MOV		ES,BX
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL
		CMP		CL,18			; 扇区号
		JBE		readfast		; CL<=18
; 读取2磁头
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH<2
; 读取10柱面
		MOV		DH,0
		ADD		CH,1
		JMP		readfast		; CH<CYLS
.ret:
		RET
		RESB	0x7dfe-$		; 填写0x00, 直到指定位置					; 0x00...
		DB		0x55, 0xaa		; 启动区最后两个字节必须为55aa
