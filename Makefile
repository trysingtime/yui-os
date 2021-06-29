default : 
	tolset\z_tools\make.exe img

ipl.bin : source\ipl.nas Makefile
	tolset\z_tools\nask.exe source\ipl.nas target\ipl.bin target\ipl.lst

helloos.img : ipl.bin Makefile
	tolset\z_tools\edimg.exe   imgin:tolset/z_tools/fdimg0at.tek   wbinimg src:target/ipl.bin len:512 from:0 to:0   imgout:target/helloos.img
	
asm :
	tolset\z_tools\make.exe -r ipl.bin

img :
	tolset\z_tools\make.exe -r helloos.img

run :
	tolset\z_tools\make.exe img
	copy target\helloos.img tolset\z_tools\qemu\fdimage0.bin
	tolset\z_tools\make.exe -C tolset\z_tools\qemu

install :
	tolset\z_tools\imgtol.com w a: target\helloos.img

clean :
	del target\ipl.bin
	del target\ipl.lst

src_only :
	tolset\z_tools\make.exe clean
	del target\helloos.img