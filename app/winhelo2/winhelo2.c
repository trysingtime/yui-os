/* 调用系统api显示窗口(使用malloc) */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_end(void);
int api_getkey(int mode);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    // 分配150*50内存大小
    // char *buf = api_malloc(150 * 50); // 使用malloc分配
    char buf[150 * 50]; // 使用alloca分配
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 150, 50, -1, "hello");
    // 显示窗口内容
    api_boxfillwin(win, 8, 36, 141, 43, 6 /*浅蓝色*/);
    api_putstrwin(win, 28, 28, 0/*黑色*/, 12, "hello, world");
    // 等待键盘输入
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            /* 回车键则break */
            break;
        }
    }    
    // 结束app
    api_end();
}
