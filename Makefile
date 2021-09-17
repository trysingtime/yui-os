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
haribote.img : target/ipl10.bin target/haribote.sys $(wildcard app/*.obj) Makefile
	$(EDIMG)   imgin:$(TOOLPATH)/fdimg0at.tek \
		wbinimg src:target/ipl10.bin len:512 from:0 to:0 \
		copy from:target/haribote.sys to:@: \
		copy from:source/ipl10.nas to:@: \
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
	-$(DEL) haribote.img
