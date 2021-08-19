; 此应用段号1003

[BITS 32]
    MOV         AL,'h'          ; 要打印的字符
    CALL        2*8:0x0f0c      ; 打印字符, 使用far-Call跨段调用操作系统(段号2)的_asm_console_putchar函数, 因此操作系统上要相应使用far-RET回应
    MOV         AL,'e'          
    CALL        2*8:0x0f0c      
    MOV         AL,'l'          
    CALL        2*8:0x0f0c      
    MOV         AL,'l'          
    CALL        2*8:0x0f0c      
    MOV         AL,'o'          
    CALL        2*8:0x0f0c      
    RETF                        ; 使用far-Call跨段调用应用函数(段号1003), 因此应用函数上要相应使用far-RET回应