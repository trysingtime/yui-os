OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj \
				fifo.obj keyboard.obj mouse.obj memory.obj layer.obj timer.obj multitask.obj \
				window.obj taskb.obj console.obj file.obj tek.obj
				
# 编译工具
ROOT	 = ../
TARGETPATH = $(ROOT)target/
TOOLPATH = $(ROOT)tolset/z_tools/
## Makefile
MAKE     = $(TOOLPATH)make.exe -r
## c to gas(cc1基于gcc改造,gcc以gas汇编语言为基础)
CC1      = $(TOOLPATH)cc1.exe -I$(ROOT)lib -Os -Wall -quiet
## gas to nask(不同的汇编语言翻译)
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
## nask to obj(目标文件, 特殊的机器语言, 不能运行)
NASK     = $(TOOLPATH)nask.exe
## obj link other to bim(链接其他文件, 做成二进制镜像文件(image), 组成一个完整的机器语言, 仍不能运行)
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
RULEFILE = $(ROOT)lib/haribote.rul
## bim to hrb(针对不同的操作系统, 给镜像文件加工, 变成真正能使用的文件)
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
## 汇编生成的bin文件和c语言生成的hrb文件相加, 生成sys文件
## ipl文件和sys文件写入镜像,生成img文件
EDIMG    = $(TOOLPATH)edimg.exe
## img文件写入磁盘
IMGTOL   = $(TOOLPATH)imgtol.com
## 字体编译
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
## 依赖库编译(obj to lib)
GOLIB    = $(TOOLPATH)golib00.exe 
## 文件操作
COPY     = copy
DEL      = del

# 编译目标
default :             
	$(MAKE) ipl20.bin
	$(MAKE) haribote.sys

# 启动程序加载器
ipl20.bin : ipl20.nas Makefile
	$(NASK) ipl20.nas $(TARGETPATH)ipl20.bin $(TARGETPATH)ipl20.lst

# 操作系统
## 系统设定+(TXT文件+汇编语言+C语言)
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B ..\target\asmhead.bin+..\target\bootpack.hrb ..\target\haribote.sys
	
## 系统设定
asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas $(TARGETPATH)asmhead.bin $(TARGETPATH)asmhead.lst

## TXT文件+C语言+汇编语言
bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(TARGETPATH)bootpack.bim stack:3136k map:$(TARGETPATH)bootpack.map \
		$(wildcard $(TARGETPATH)*.obj)
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) $(TARGETPATH)bootpack.bim $(TARGETPATH)bootpack.hrb 0

## TXT文件
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt $(TARGETPATH)hankaku.bin
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) $(TARGETPATH)hankaku.bin $(TARGETPATH)hankaku.obj _hankaku

## C语言(c->gas->nas->obj)
%.obj : %.nas Makefile
	$(NASK) $(TARGETPATH)$< $(TARGETPATH)$@ $(TARGETPATH)$*.lst
%.nas : %.gas Makefile
	$(GAS2NASK) $(TARGETPATH)$< $(TARGETPATH)$@
%.gas : %.c bootpack.h Makefile
	$(CC1) -o $(TARGETPATH)$@ $<

## 汇编语言(nas->obj)
naskfunc.obj : naskfunc.nas Makefile
	$(NASK) naskfunc.nas $(TARGETPATH)naskfunc.obj $(TARGETPATH)naskfunc.lst

clean :
	-$(DEL) ..\target\*.lst
	-$(DEL) ..\target\*.gas
	-$(DEL) ..\target\*.nas
	-$(DEL) ..\target\*.obj
	-$(DEL) ..\target\*.map
	-$(DEL) ..\target\*.bim
	-$(DEL) ..\target\*.hrb

src_only :
	$(MAKE) clean
	-$(DEL) ..\target\*.bin
	-$(DEL) ..\target\haribote.sys
