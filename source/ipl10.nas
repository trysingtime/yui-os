; haribote-ipl
; TAB=4

CYLS	EQU		10			; 读取柱面数
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
readloop:
		MOV		SI,0			; 记录失败次数
retry:
		MOV		AH,0x02			; 0x02:读盘;0x03:写盘;0x04:校验;0x0c:寻道;0x00:系统复位
		MOV		AL,1			; 一个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS(返回FLACS.CF, 0:没有错误(AH置为0), 1:有错误(AH置为错误码))
		JNC		next
; 读取重试
		ADD		SI,1
		CMP		SI,5
		JAE		error			; SI>=5
		MOV		AH,0x00			; 系统复位
		MOV		DL,0x00			; A驱动器
		INT		0x13
		JMP		retry
next:
; 读取18扇区
		MOV		AX,ES			; 将内存地址后移0x200,ES无法使用ADD,通过AX实现
		ADD		AX,0x0020		; [ES:BX]=ESx16+BX, 因此后移0x200相当于ES后移0x0020
		MOV		ES,AX
		ADD		CL,1
		CMP		CL,18			; 扇区号
		JBE		readloop		; CL<=18
; 读取2磁头
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		; DH<2
; 读取10柱面
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS
		JB		readloop		; CH<CYLS

; 读取完毕, 跳转到haribote.sys
		MOV		[0x0ff0],CH		; 将CYLS的值写到内存地址0x0ff0中, 后续haribote.sys可以获取该值
		JMP		0xc200			; haribote.sys保存在磁盘的0x004200位置, 磁盘的前512字节(0x0200)被自动读取到内存的0x7c00~0x7dff, 
								; 后续内容被上面手动读取0x8200, 因此haribote.sys读取到内存后位于0x8200+(0x4200-0x0200)=0xc200

error:
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

		RESB	0x7dfe-$		; 填写0x00, 直到指定位置					; 0x00...
		DB		0x55, 0xaa		; 启动区最后两个字节必须为55aa
