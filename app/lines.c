/* 调用系统api绘制直线 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);
void api_closewin(int win);
void api_end(void);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(160 * 100); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 160, 100, -1, "lines");
    // 绘制
    int i;
    for (i = 0; i < 8; i++) {
        api_linewin(win, 8, 26, 77, i * 9 + 26, i);
        api_linewin(win, 88, 26, i * 9 + 88,89, i);
    }
    // 刷新窗口图层
    api_refreshwin(win, 6, 26, 154, 90);
    // 关闭窗口图层
    api_closewin(win);
    // 结束app
    api_end();
}
