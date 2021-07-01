TOOLPATH = tolset/z_tools/
INCPATH  = tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
# c to gas(cc1基于gcc改造,gcc以gas汇编语言为基础)
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
# gas to nask(不同的汇编语言翻译)
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
# nask to obj(目标文件, 特殊的机器语言, 不能运行)
NASK     = $(TOOLPATH)nask.exe
# obj link other to bim(链接其他文件, 做成二进制镜像文件(image), 组成一个完整的机器语言, 仍不能运行)
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
# bim to hrb(针对不同的操作系统, 给镜像文件加工, 变成真正能使用的文件)
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

default : 
	$(MAKE) img

ipl10.bin : source\ipl10.nas Makefile
	$(NASK) source\ipl10.nas target\ipl10.bin target\ipl10.lst

asmhead.bin : source\asmhead.nas Makefile
	$(NASK) source\asmhead.nas target\asmhead.bin target\asmhead.lst

bootpack.gas : source\bootpack.c Makefile
	$(CC1) -o target\bootpack.gas source\bootpack.c

bootpack.nas : bootpack.gas Makefile
	$(GAS2NASK) target\bootpack.gas target\bootpack.nas

bootpack.obj : bootpack.nas Makefile
	$(NASK) target\bootpack.nas target\bootpack.obj target\bootpack.lst

naskfunc.obj : source\naskfunc.nas Makefile
	$(NASK) source\naskfunc.nas target\naskfunc.obj target\naskfunc.lst

# 3MB+64KB=3136KB
bootpack.bim : bootpack.obj naskfunc.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:target\bootpack.bim stack:3136k \
		map:bootpack.map target\bootpack.obj target\naskfunc.obj

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) target\bootpack.bim target\bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B target\asmhead.bin+target\bootpack.hrb target\haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	$(EDIMG)   imgin:tolset/z_tools/fdimg0at.tek \
		wbinimg src:target/ipl10.bin len:512 from:0 to:0 \
		copy from:target/haribote.sys to:@: \
		imgout:target/haribote.img

img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) target\haribote.img tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C tolset\z_tools\qemu

install :
	$(IMGTOL) w a: target\haribote.img

clean :
	-$(DEL) target\*.bin
	-$(DEL) target\*.lst
	-$(DEL) target\*.gas
	-$(DEL) target\*.obj
	-$(DEL) target\bootpack.nas
	-$(DEL) target\bootpack.map
	-$(DEL) target\bootpack.bim
	-$(DEL) target\bootpack.hrb
	-$(DEL) target\haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) target\haribote.img