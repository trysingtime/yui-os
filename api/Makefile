OBJS_API = 	api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj \
			api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj \
			api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj \
			api019.obj api020.obj api021.obj api022.obj api023.obj api024.obj \
			api025.obj api026.obj api027.obj \
			alloca.obj

# 编译工具
ROOT	 = ../
TOOLPATH = $(ROOT)tolset/z_tools/
## Makefile
MAKE     = $(TOOLPATH)make.exe -r
## nask to obj(目标文件, 特殊的机器语言, 不能运行)
NASK     = $(TOOLPATH)nask.exe
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
