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
