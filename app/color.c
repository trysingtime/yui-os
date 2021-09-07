/* 显示16基本色+6阶RGB色(6*6*6=216)种颜色 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_initmalloc(void);
char *api_malloc(int size);
int api_getkey(int mode);
void api_end(void);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(144 * 164); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 144, 164, -1, "color");
    // 遍历128*128的显示区域像素点, 分割成43*43的方块, 方块内部颜色一致
    int x, y;
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            int r = x * 2; // 共128个像素点要显示256亮度, 因此*2
            int g = y * 2;
            int b = 0;
            buf[(x + 8) + (y + 28) * 144] = 16 + (r / 43) + (g / 43) * 6 + (b / 43) * 36; // 每43个像素点才更换一种颜色
        }
    }
    // 刷新窗口图层
    api_refreshwin(win, 8, 28, 136, 156);
    // 等待键盘按下任何按键退出
    api_getkey(1);
    // 结束app
    api_end();
}
