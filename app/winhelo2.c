/* 调用系统api显示窗口(使用malloc) */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_end(void);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(150 * 50); // 分配150*50内存大小
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 150, 50, -1, "hello");
    // 显示窗口内容
    api_boxfillwin(win, 8, 36, 141, 43, 6 /*浅蓝色*/);
    api_putstrwin(win, 28, 28, 0/*黑色*/, 12, "hello, world");
    // 结束app
    api_end();
}
