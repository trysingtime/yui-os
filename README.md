# hello-os
learn to make os

# 内存分布
0x00000000 - 0x000fffff : (1MB)虽然在启动中会多次使用, 但之后会变空
0x00100000 - 0x00267fff : (1440KB)用于保存软盘内容
0x00268000 - 0x0026f7ff : (30KB)空
0x0026f800 - 0x0026ffff : (2KB)IDT
0x00270000 - 0x0027ffff : (64KB)GDT
0x00280000 - 0x002fffff : (512KB)bootpack.hrb
0x00300000 - 0x003fffff : (1MB)栈及其他
    0x003c0000 - 0x003c8fff : (32KB)内存管理
0x00400000 -            : 空
