# 字体文件
chinese.org: 基于中文编码GB2312的HZK16点阵字体截取版(截取前55区(55*94*32=165440字节))
chinese.fnt: chinese.org的压缩版
nihongo.org: 基于日语编码Shift-JIS和EUC的点阵字体截取版(截取前47区(47*94*32=141376字节))
nihongo.fnt: nihongo.org的压缩版

# 压缩字体文件
..\tolset\z_tools\bim2bin.exe -osacmp in:nihongo.org out:nihongo.fnt
..\tolset\z_tools\bim2bin.exe -osacmp in:chinese.org out:chinese.fnt

# GB2312
- 01~09区: 非汉字
- 10~15区: 空白
- 16~55区: 一级汉字
- 56~87区: 二级汉字
- 88~94区: 空白

# JIS
- 01~13区: 非汉字
- 14~15区: 第三水准汉字
- 16~47区: 第一水准汉字
- 48~84区: 第二水准汉字
- 88~94区: 第三水准汉字
  