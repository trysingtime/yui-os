/* 调用系统api画星星 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_point(int win, int x, int y, int col);
void api_end(void);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(150 * 100); // 分配150*50内存大小
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 150, 100, -1, "star1");
    // 显示窗口内容
    api_boxfillwin(win, 6, 26, 143, 93, 0 /*黑色*/);
    api_point(win, 75, 59, 3/*黄色*/);
    // 结束app
    api_end();
}
