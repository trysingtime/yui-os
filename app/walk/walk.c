/* 调用系统api移动星星 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);
int api_getkey(int mode);
void api_closewin(int win);
void api_end(void);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(160 * 100); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 160, 100, -1, "walk");
    // 绘制黑色背景
    api_boxfillwin(win, 4, 24, 155, 95, 0 /*黑色*/);
    // 绘制星星
    char *target = "*";
    int x = 76;
    int y = 56;
    api_putstrwin(win, x, y, 3/*黄色*/, 1, target);
    // 等待键盘输入
    int i;
    for (;;) {
        i = api_getkey(1);
        // 隐藏旧星星
        api_putstrwin(win, x, y, 0/*黑色*/, 1, target);
        // 通过数字键4/6/8/2/5移动星星
        if (i == '4' && x >   4) { x -= 8; }
        if (i == '6' && x < 148) { x += 8; }
        if (i == '8' && y >  24) { y -= 8; }
        if (i == '2' && y <  80) { y += 8; }
        if (i == '5') { x = 76; y = 56; } // 数字键'5'回到原点
        if (i == 0x0a) { break; } // 回车键退出
        // 绘制新星星
        api_putstrwin(win, x, y, 3/*黄色*/, 1, target);
    }
    // 关闭窗口图层
    api_closewin(win);
    // 结束app
    api_end();
}
