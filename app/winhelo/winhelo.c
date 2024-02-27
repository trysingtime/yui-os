/* 调用系统api显示窗口 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
int api_getkey(int mode);
void api_end(void);

//char buf[150 * 50]; // 使用静态变量将会编译为obj占用代码段内存
void HariMain(void) {
    char buf[150 * 50]; // 使用内部变量使用栈空间, 此处大于4KB因此编译器将会调用_alloca函数
    int win; // 返回的图层地址
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfillwin(win, 8, 36, 141, 43, 3 /*黄色*/);
    api_putstrwin(win, 28, 28, 0/*黑色*/, 12, "hello, world");
    // 等待键盘输入
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            /* 回车键则break */
            break;
        }
    }
    api_end();
}
