/* 调用系统api绘制星星 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_end(void);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);

int rand(void); // 产生0~32767之间的随机数

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(150 * 100); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 150, 100, -1, "star1");
    // 显示窗口内容
    api_boxfillwin(win, 6, 26, 143, 93, 0 /*黑色*/);
    // 绘制
    int i;
    for (i = 0; i < 50; i++) {
        int x = (rand() % 137) + 6; // 获取(0~137)+6间的随机数
        int y = (rand() % 67) + 26; // 获取(0~67)+26间的随机数
        api_point(win, x, y, 3/*黄色*/);
    }
    // 刷新窗口图层
    api_refreshwin(win, 6, 26, 144, 94);
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
