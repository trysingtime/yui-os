; 通过CALL直接调用io_cli()使系统停止中断
[INSTRSET "i486p"]          ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                   ; 设定成32位机器语言模式
    CALL    2*8:0x0dd2      ; 通过bootpack.map查到io_cli()地址为0xdd2, 直接调用使系统停止中断
    MOV     EDX,4           
    INT     0x40            ; 结束app, 新版本使用了RETF来调用app函数, app不能再使用far-RET回应, 而是直接调用end_app结束程序直接返回到cmd_app()
