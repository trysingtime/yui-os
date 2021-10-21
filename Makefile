# 编译工具
ROOT	 = ./
TOOLPATH = $(ROOT)tolset/z_tools/
## Makefile
MAKE     = $(TOOLPATH)make.exe -r
## ipl文件和sys文件写入镜像,生成img文件
EDIMG    = $(TOOLPATH)edimg.exe
## img文件写入磁盘
IMGTOL   = $(TOOLPATH)imgtol.com
## 文件操作
COPY     = copy
DEL      = del

# 编译目标
## 不编译直接运行
default :             
	$(MAKE) haribote.img
run :
	$(MAKE) haribote.img
	$(COPY) haribote.img tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C $(TOOLPATH)qemu

## 编译系统+API+应用程序并运行
full : system application
	$(MAKE) haribote.img
run_full :  system application
	$(MAKE) haribote.img
	$(COPY) haribote.img tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C $(TOOLPATH)qemu

install :
	$(MAKE) haribote.img
	$(IMGTOL) w a: haribote.img

## 启动程序加载器+操作系统+应用程序
haribote.img : target/ipl20.bin target/haribote.sys $(wildcard app/*.obj) Makefile
	$(EDIMG)   imgin:$(TOOLPATH)/fdimg0at.tek \
		wbinimg src:target/ipl20.bin len:512 from:0 to:0 \
		copy from:target/haribote.sys to:@: \
		copy from:source/ipl20.nas to:@: \
		copy from:font/chinese.fnt to:@: \
		copy from:font/nihongo.fnt to:@: \
		copy from:make.bat to:@: \
		copy from:app/a/a.hrb to:@: \
		copy from:app/hello3/hello3.hrb to:@: \
		copy from:app/hello4/hello4.hrb to:@: \
		copy from:app/hello5/hello5.hrb to:@: \
		copy from:app/winhelo/winhelo.hrb to:@: \
		copy from:app/winhelo2/winhelo2.hrb to:@: \
		copy from:app/star1/star1.hrb to:@: \
		copy from:app/stars/stars.hrb to:@: \
		copy from:app/lines/lines.hrb to:@: \
		copy from:app/walk/walk.hrb to:@: \
		copy from:app/noodle/noodle.hrb to:@: \
		copy from:app/beepdown/beepdown.hrb to:@: \
		copy from:app/color/color.hrb to:@: \
		copy from:app/color2/color2.hrb to:@: \
		copy from:app/sosu/sosu.hrb to:@: \
		copy from:app/sosu2/sosu2.hrb to:@: \
		copy from:app/sosu3/sosu3.hrb to:@: \
		copy from:app/type/type.hrb to:@: \
		copy from:app/iroha/iroha.hrb to:@: \
		copy from:app/chklang/chklang.hrb to:@: \
		copy from:app/notrec/notrec.hrb to:@: \
		copy from:app/bball/bball.hrb to:@: \
		copy from:app/invader/invader.hrb to:@: \
		copy from:app/calc/calc.hrb to:@: \
		copy from:app/notepad/notepad.hrb to:@: \
		imgout:haribote.img

## 启动程序加载器+操作系统
system : 
	$(MAKE) -C source

## 应用程序
application : 
	$(MAKE) -C api
	$(MAKE) -C app/a
	$(MAKE) -C app/hello3
	$(MAKE) -C app/hello4
	$(MAKE) -C app/hello5
	$(MAKE) -C app/winhelo
	$(MAKE) -C app/winhelo2
	$(MAKE) -C app/star1
	$(MAKE) -C app/stars
	$(MAKE) -C app/lines
	$(MAKE) -C app/walk
	$(MAKE) -C app/noodle
	$(MAKE) -C app/beepdown
	$(MAKE) -C app/color
	$(MAKE) -C app/color2
	$(MAKE) -C app/sosu
	$(MAKE) -C app/sosu2
	$(MAKE) -C app/sosu3
	$(MAKE) -C app/type
	$(MAKE) -C app/iroha
	$(MAKE) -C app/chklang
	$(MAKE) -C app/notrec
	$(MAKE) -C app/bball
	$(MAKE) -C app/invader
	$(MAKE) -C app/calc
	$(MAKE) -C app/notepad

clean :
	$(MAKE) -C source		clean
	$(MAKE) -C api			clean
	$(MAKE) -C app/a		clean
	$(MAKE) -C app/hello3	clean
	$(MAKE) -C app/hello4	clean
	$(MAKE) -C app/hello5	clean
	$(MAKE) -C app/winhelo	clean
	$(MAKE) -C app/winhelo2	clean
	$(MAKE) -C app/star1	clean
	$(MAKE) -C app/stars	clean
	$(MAKE) -C app/lines	clean
	$(MAKE) -C app/walk		clean
	$(MAKE) -C app/noodle	clean
	$(MAKE) -C app/beepdown	clean
	$(MAKE) -C app/color	clean
	$(MAKE) -C app/color2	clean
	$(MAKE) -C app/sosu		clean
	$(MAKE) -C app/sosu2	clean
	$(MAKE) -C app/sosu3	clean
	$(MAKE) -C app/type		clean
	$(MAKE) -C app/iroha	clean
	$(MAKE) -C app/chklang	clean
	$(MAKE) -C app/notrec	clean
	$(MAKE) -C app/bball	clean
	$(MAKE) -C app/invader	clean
	$(MAKE) -C app/calc		clean
	$(MAKE) -C app/notepad	clean

src_only :
	$(MAKE) -C source		src_only
	$(MAKE) -C api			src_only
	$(MAKE) -C app/a		src_only
	$(MAKE) -C app/hello3	src_only
	$(MAKE) -C app/hello4	src_only
	$(MAKE) -C app/hello5	src_only
	$(MAKE) -C app/winhelo	src_only
	$(MAKE) -C app/winhelo2	src_only
	$(MAKE) -C app/star1	src_only
	$(MAKE) -C app/stars	src_only
	$(MAKE) -C app/lines	src_only
	$(MAKE) -C app/walk		src_only
	$(MAKE) -C app/noodle	src_only
	$(MAKE) -C app/beepdown	src_only
	$(MAKE) -C app/color	src_only
	$(MAKE) -C app/color2	src_only
	$(MAKE) -C app/sosu		src_only
	$(MAKE) -C app/sosu2	src_only
	$(MAKE) -C app/sosu3	src_only
	$(MAKE) -C app/type		src_only
	$(MAKE) -C app/iroha	src_only
	$(MAKE) -C app/chklang	src_only
	$(MAKE) -C app/notrec	src_only
	$(MAKE) -C app/bball	src_only
	$(MAKE) -C app/invader	src_only
	$(MAKE) -C app/calc		src_only
	$(MAKE) -C app/notepad	src_only
	-$(DEL) haribote.img
