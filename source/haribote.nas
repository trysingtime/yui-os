; haribote-os
; TAB=4

; 指定一段内存地址, 用以缓存BOOT_INFO
CYLS	EQU		0x0ff0			    ; 读取的柱面数
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			    ; 几位色
SCRNX	EQU		0x0ff4			    ; 分辨率X
SCRNY	EQU		0x0ff6			    ; 分辨率Y
VRAM	EQU		0x0ff8			    ; 图像缓冲区开始地址

        ORG     0xc200

; 当前CPU为16位模式, 后续要切换到32位模式, 但BIOS是16位机器语言写的, 切换后无法再调用BIOS功能, 因此切换前把需调用BIOS功能的提前完成
; 通过BIOS设置显卡模式(video mode)
        MOV     AL,0x13             ; 0x03:16色字符模式,80x25;0x12:VGA图形模式, 640X480X4位彩色; 0x13:VGA图形模式,320x200x8位彩色; 0x6a: 扩展VGA图形模式, 800x600x4位彩色
        MOV     AH,0x00
        INT     0x10

; 保存BOOT_INFO
        MOV     BYTE [VMODE],8      ; 几位色
        MOV     WORD [SCRNX],320    ; 分辨率X
        MOV     WORD [SCRNY],200    ; 分辨率Y
        MOV     DWORD [VRAM],0x000a0000     ; 图像缓冲区开始地址(不同显卡模式对应不同VRAM地址, 0x13对应0xa0000~0xaffff)

; 通过BIOS取得键盘各种LED指示灯的状态
        MOV     AH,0x02
        INT     0x16
        MOV     [LEDS],AL

fin:
        HLT
        JMP     fin