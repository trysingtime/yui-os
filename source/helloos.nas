; hello-os
; TAB=4

; 标准FAT12格式软盘
	DB		0xeb, 0x4e, 0x90
	DB		"HELLOIPL"		; 启动区名称(字符串8字节)					; 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x49, 0x50, 0x4c
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
	DB		"HELLO-OS   "	; 磁盘的名称(11字节)						; 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x2d, 0x4f, 0x53, 0x20, 0x20, 0x20
	DB		"FAT12   "		; 磁盘格式名称(8字节)						; 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20
	RESB	18				; 空出18字节								; 0x00...

; 程序主体
	DB		0xb8, 0x00, 0x00, 0x8e, 0xd0, 0xbc, 0x00, 0x7c
	DB		0x8e, 0xd8, 0x8e, 0xc0, 0xbe, 0x74, 0x7c, 0x8a
	DB		0x04, 0x83, 0xc6, 0x01, 0x3c, 0x00, 0x74, 0x09
	DB		0xb4, 0x0e, 0xbb, 0x0f, 0x00, 0xcd, 0x10, 0xeb
	DB		0xee, 0xf4, 0xeb, 0xfd
	
; 信息显示部分	
	DB		0x0a, 0x0a		; 两个换行
	DB		"hello, world"	; 打印信息									; 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64
	DB		0x0a			; 换行
	DB		0x00

	RESB	0x1fe-$			; 填写0x00, 直到0x001fe						; 0x00...
	DB		0x55, 0xaa		; 启动区最后两个字节必须为55aa

; 启动区以外部分输出
	DB	0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
	RESB	4600
	DB	0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
	RESB	1469432
