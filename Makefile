TOOLPATH = tolset/z_tools/
MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

default : 
	$(MAKE) img

ipl.bin : source\ipl.nas Makefile
	$(NASK) source\ipl.nas target\ipl.bin target\ipl.lst

haribote.sys : source\haribote.nas Makefile
	$(NASK) source\haribote.nas target\haribote.sys target\haribote.lst

haribote.img : ipl.bin haribote.sys Makefile
	$(EDIMG)   imgin:tolset/z_tools/fdimg0at.tek \
		wbinimg src:target/ipl.bin len:512 from:0 to:0 \
		copy from:target/haribote.sys to:@: \
		imgout:target/haribote.img
	
asm :
	$(MAKE) -r ipl.bin

img :
	$(MAKE) -r haribote.img

run :
	$(MAKE) img
	$(COPY) target\haribote.img tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C tolset\z_tools\qemu

install :
	$(IMGTOL) w a: target\haribote.img

clean :
	-$(DEL) target\ipl.bin
	-$(DEL) target\ipl.lst
	-$(DEL) target\haribote.sys
	-$(DEL) target\haribote.lst

src_only :
	$(MAKE) clean
	-$(DEL) target\haribote.img