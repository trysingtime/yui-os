OBJS_API = 	api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj \
			api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj \
			api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj \
			api019.obj api020.obj

# 编译工具
ROOT	 = ../
TOOLPATH = $(ROOT)tolset/z_tools/
## Makefile
MAKE     = $(TOOLPATH)make.exe -r
## c to gas(cc1基于gcc改造,gcc以gas汇编语言为基础)
INCPATH  = $(ROOT)tolset/z_tools/haribote/
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
## gas to nask(不同的汇编语言翻译)
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
## nask to obj(目标文件, 特殊的机器语言, 不能运行)
NASK     = $(TOOLPATH)nask.exe
## obj link other to bim(链接其他文件, 做成二进制镜像文件(image), 组成一个完整的机器语言, 仍不能运行)
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
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
	$(MAKE) api.lib

api.lib : $(OBJS_API) Makefile
	$(GOLIB) $(OBJS_API) out:$@

%.obj : %.nas Makefile
	$(NASK) $< $@ $*.lst

clean :
	-$(DEL) *.lst
	-$(DEL) *.obj

src_only :
	$(MAKE) clean
	-$(DEL) api.lib
