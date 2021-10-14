/* 显示单个字符 */
void api_putchar(int c);
/* 显示字符串(以0结尾) */
void api_putstr0(char *s);
/* 显示字符串(指定长度) */
void api_putstr1(char *s, int l);
/* 强制结束app */
void api_end(void);
/* 显示窗口(edx:5,ebx:窗口内容地址,esi:窗口宽度,edi:窗口高度,eax:窗口颜色和透明度,ecx:窗口标题,返回值放入eax) */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
/* 窗口显示字符串(edx:6,ebx:窗口内容地址,esi:显示的x坐标,edi:显示的y坐标,eax:颜色,ecx:字符长度,ebp:字符串) */
void api_putstrwin(int win, int x, int y, int col, int len, char *str);void api_putstr1(char *s, int l);
/* 窗口显示方块(edx:7,ebx:窗口内容地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色) */
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
/* 初始化app内存控制器(edx:8,ebx:内存控制器地址,eax:管理的内存空间起始地址,ecx:管理的内存空间字节数) */
void api_initmalloc(void);
/* 分配指定大小的内存(edx:9,ebx:内存控制器地址,eax:分配的内存空间起始地址,ecx:分配的内存空间字节数, 返回值放入eax) */
char *api_malloc(int size);
/* 释放指定起始地址和大小的内存(edx:10,ebx:内存控制器地址,eax:释放的内存空间起始地址,ecx:释放的内存空间字节数) */
void api_free(char *addr, int size);
/* 在窗口中画点(edx:11,ebx:窗口图层地址,esi:x坐标,edi:y坐标,eax:颜色) */
void api_point(int win, int x, int y, int col);
/* 窗口图层刷新(edx:12,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1) */
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
/* 窗口绘制直线(edx:13,ebx:窗口图层地址,eax:x0,ecx:y0,esi:x1,edi:y1,ebp:颜色) */
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
/* 关闭窗口图层(edx:14,ebx:窗口图层地址) */
void api_closewin(int win);
/* 获取键盘输入(edx:15,eax:是否休眠等待至中断输入,返回值放入eax) */
int api_getkey(int mode);
/* 获取定时器(edx:16,ebx:定时器地址,返回值放入eax) */
int api_alloctimer(void);
/* 设置定时器发送的数据(edx:17,ebx:定时器地址,eax:数据) */
void api_inittimer(int timer, int data);
/* 设置定时器倒计时(edx:18,ebx:定时器地址,eax:时间(timeout/100s)) */
void api_settimer(int timer, int time);
/* 释放定时器(edx:19,ebx:定时器地址) */
void api_freetimer(int timer);
/* 显示蜂鸣器发声(edx:20,eax:声音频率(mHz, 4400000mHz = 440Hz, 频率为0标识停止发声)) */
void api_beep(int tone);
/* 打开文件(edx:21,ebx:文件名,eax(返回值):文件缓冲区地址) */
int api_fopen(char *fname);
/* 关闭文件(edx:22,eax:文件缓冲区地址) */
void api_fclose(int fhandle);
/* 文件定位(edx:23,eax:文件缓冲区地址,ecx:定位模式(0:定位起点为文件开头,1:定位起点为当前访问位置,2:定位起点为文件末尾)),ebx:定位偏移量 */
void api_fseek(int fhandle, int offset, int mode);
/* 获取文件大小(edx:24,eax:文件缓冲区地址,ecx:文件大小获取模式(0:普通文件大小,1:当前读取位置到文件开头起算的偏移量,2:当前读取位置到文件末尾起算的偏移量),eax(返回值):文件大小) */
int api_fsize(int fhandle, int mode);
/* 文件读取(edx:25,eax:文件缓冲区地址,ebx:读取文件目的地址,ecx:最大读取字节数,eax(返回值):本次读取到的字节数) */
int api_fread(char *buf, int maxsize, int fhandle);
/* 获取控制台当前指令(edx:26,eax:命令行缓冲区地址,ecx:最大存放字节数,eax(返回值):实际存放字节数) */
int api_cmdline(char *buf, int maxsize);
/* 获取控制台当前语言模式(edx:27,eax(返回值): 语言模式) */
int api_getlang(void);
