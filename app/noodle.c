#include <stdio.h>
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
int api_getkey(int mode);
int api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_end(void);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(160 * 50); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 150, 50, -1, "noodle");
    // 初始化定时器
    int timer = api_alloctimer();
    api_inittimer(timer, 128); // 倒计时结束发送128到fifo
    // 每秒一次遍历显示时间
    int hou = 0, min = 0, sec = 0;
    for (;;) {
        char s[12];
        sprintf(s, "%5d:%02d:%02d", hou, min, sec);
        api_boxfillwin(win, 28, 27, 115, 41, 7/*白色*/);
        api_putstrwin(win, 28, 27, 0/*黑色*/, 11, s);

        api_settimer(timer, 100); // 1s
        // 休眠等待中断(参数为1: 休眠直到中断输入, 0: 不休眠返回-1)
        if (api_getkey(1) != 128) {
            // 按下任意键结束app
            break;
        }
        sec++;
        if (sec == 60) {
            sec = 0;
            min++;
            if (min == 60) {
                min = 0;
                hou++;
            }
        }
    }
    // 结束app
    api_end();
}
