if not exist %cd%\target md target
copy source\helloos.img target\helloos.img
copy target\helloos.img tolset\z_tools\qemu\fdimage0.bin
tolset\z_tools\make.exe -C tolset\z_tools\qemu
del tolset\z_tools\qemu\fdimage0.bin
