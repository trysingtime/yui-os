; haribote-os
; TAB=4

        ORG     0xc200

; 设置显卡模式(video mode)
        MOV     AL,0x13         ; 0x03:16色字符模式,80x25;0x12:VGA图形模式, 640X480X4位彩色; 0x13:VGA图形模式,320x200x8位彩色; 0x6a: 扩展VGA图形模式, 800x600x4位彩色
        MOV     AH,0x00
        INT     0x10
fin:
        HLT
        JMP     fin