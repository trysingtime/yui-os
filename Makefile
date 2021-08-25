# 变量
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj \
				fifo.obj keyboard.obj mouse.obj memory.obj layer.obj timer.obj multitask.obj \
				window.obj taskb.obj console.obj file.obj
OBJS_BOOTPACK_TARGET = target\bootpack.obj target\naskfunc.obj target\hankaku.obj \
						target\graphic.obj target\dsctbl.obj target\int.obj target\fifo.obj \
						target\keyboard.obj target\mouse.obj target\memory.obj target\layer.obj \
						target\timer.obj target\multitask.obj target\window.obj target\taskb.obj \
						target\console.obj target\file.obj

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
RULEFILE = $(TOOLPATH)haribote/haribote.rul
# bim to hrb(针对不同的操作系统, 给镜像文件加工, 变成真正能使用的文件)
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
# 汇编生成的bin文件和c语言生成的hrb文件相加, 生成sys文件
# ipl文件和sys文件写入镜像,生成img文件
EDIMG    = $(TOOLPATH)edimg.exe
# img文件写入磁盘
IMGTOL   = $(TOOLPATH)imgtol.com

# 字体编译
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
COPY     = copy
DEL      = del

# 单元规则
default :             
	$(MAKE) img

# 应用程序
app : a.hrb hello.hrb hello2.hrb hello3.hrb crack1.hrb crack2.hrb crack3.hrb crack4.hrb crack5.hrb
## 汇编语言
%.hrb : app\%.nas Makefile
	$(NASK) $< target\$@ target\$*.lst
%.hrb : crack\%.nas Makefile
	$(NASK) $< target\$@ target\$*.lst

## 汇编语言API
a_nask.obj : source\a_nask.nas Makefile
	$(NASK) source\a_nask.nas target\a_nask.obj target\a_nask.lst

## C语言(通过通配符c->gas->nas->obj)
%.gas : app\%.c Makefile
	$(CC1) -o target\$@ $<
%.gas : crack\%.c Makefile
	$(CC1) -o target\$@ $<

## 汇编语言API+C语言
%.bim : %.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:target\$*.bim map:target\$*.map target\$*.obj target\a_nask.obj
%.hrb : %.bim Makefile
	$(BIM2HRB) target\$*.bim target\$*.hrb 0

# 启动程序加载器
ipl10.bin : source\ipl10.nas Makefile
	$(NASK) source\ipl10.nas target\ipl10.bin target\ipl10.lst

# 操作系统
## 系统设定
asmhead.bin : source\asmhead.nas Makefile
	$(NASK) source\asmhead.nas target\asmhead.bin target\asmhead.lst

## TXT文件
hankaku.bin : source\hankaku.txt Makefile
	$(MAKEFONT) source\hankaku.txt target\hankaku.bin
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) target\hankaku.bin target\hankaku.obj _hankaku

## 汇编语言
naskfunc.obj : source\naskfunc.nas Makefile
	$(NASK) source\naskfunc.nas target\naskfunc.obj target\naskfunc.lst

## C语言(通过通配符c->gas->nas->obj)
%.gas : source\%.c Makefile
	$(CC1) -o target\$@ $<

## TXT文件+汇编语言+C语言
bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:target\bootpack.bim stack:3136k map:target\bootpack.map \
		$(OBJS_BOOTPACK_TARGET)
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) target\bootpack.bim target\bootpack.hrb 0

## 系统设定+(TXT文件+汇编语言+C语言)
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B target\asmhead.bin+target\bootpack.hrb target\haribote.sys

# 启动程序加载器+操作系统+应用程序
haribote.img : ipl10.bin haribote.sys app Makefile
	$(EDIMG)   imgin:tolset/z_tools/fdimg0at.tek \
		wbinimg src:target/ipl10.bin len:512 from:0 to:0 \
		copy from:target/haribote.sys to:@: \
		copy from:target/a.hrb to:@: \
		copy from:target/hello.hrb to:@: \
		copy from:target/hello2.hrb to:@: \
		copy from:target/hello3.hrb to:@: \
		copy from:target/crack1.hrb to:@: \
		copy from:target/crack2.hrb to:@: \
		copy from:target/crack3.hrb to:@: \
		copy from:target/crack4.hrb to:@: \
		copy from:target/crack5.hrb to:@: \
		copy from:source/ipl10.nas to:@: \
		copy from:make.bat to:@: \
		imgout:target/haribote.img

# 通配符规则
%.nas : %.gas Makefile
	$(GAS2NASK) target\$< target\$@
%.obj : %.nas Makefile
	$(NASK) target\$< target\$@ target\$*.lst

# 汇总规则
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
	-$(DEL) target\*.nas
	-$(DEL) target\*.obj
	-$(DEL) target\*.map
	-$(DEL) target\*.bim
	-$(DEL) target\*.hrb
	-$(DEL) target\haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) target\haribote.img
