[FORMAT "WCOFF"]                    ; 输出格式(需要编译成obj, 所以设定为WCOFF模式)
[INSTRSET "i486p"]                  ; 指定486CPU(32位)使用, 8086->80186->286(16位)->386(32位)->486(带有缓存)->Pentium->PentiumPro->PentiumII
[BITS 32]                           ; 设定成32位机器语言模式

; 编译成obj文件的信息
[FILE "api.nas"]                    ; 源文件名
        GLOBAL _api_initmalloc      ; 函数名
; 代码段
[SECTION .text]                     
; 初始化app内存控制器(edx:8,ebx:内存控制器地址,eax:管理的内存空间起始地址,ecx:管理的内存空间字节数)
_api_initmalloc:        ; void api_initmalloc(void)
        PUSH            EBX
        MOV             EDX,8
        MOV             EBX,[CS:0x0020]         ; 内存控制器地址(数据段相对地址)=malloc内存空间起始地址=hrb文件的0x0020=app代码段的0x0020
        MOV             EAX,EBX     
        ADD             EAX,32*1024             ; 管理的内存空间起始地址, 内存控制器占用了开头的32k, 因此需要偏移32k  
        MOV             ECX,[CS:0x0000]         ; hrb文件的0x0000存放app数据段大小(编译时指定的malloc大小+栈大小, 请确保app的malloc大小超过32K!!!)
        SUB             ECX,EAX                 ; 管理的内存空间字节数=app数据段大小-内存控制器地址(数据段相对地址)-内存控制器大小32k
        INT             0x40
        POP             EBX
        RET
