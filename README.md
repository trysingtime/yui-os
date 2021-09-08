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

# .hrb文件
由 bim2hrb 生成的.hrb 文件,开头的 36 个字节不是程序,而是存放了下列信息：
地址            信息内容
0x0000(DWORD)   请求操作系统为应用程序准备的数据段的大小(编译时指定的malloc大小+栈大小)
0x0004(DWORD)   “Hari”(.hrb 文件的标志)
0x0008(DWORD)   数据段内预备空间的大小(无用,置0)
0x000c(DWORD)   ESP 初值 & 数据部分传送目的地址(栈顶, 栈大小由obj2bim参数如(stack:1k)决定)
0x0010(DWORD)   hrb 文件内数据部分的大小
0x0014(DWORD)   hrb 文件内数据的起始地址
0x0018(DWORD)   0xe9000000(E9为Near Jump(近跳转), 后面地址为相对位移)
0x001c(DWORD)   应用程序入口地址 - 0x20(相对位移, 0x18(实际为0x1b)的JMP指令执行后EIP为0x20, 因此需要减去0x20得到相对位移)
0x0020(DWORD)   malloc 空间起始地址(数据段的相对地址, 而不是代码段)
