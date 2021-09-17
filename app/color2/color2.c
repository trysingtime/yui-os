/* 
    显示16基本色+6阶亮度RGB色+RGB中间色. 共16+21*21*21=9277种颜色
    4个像素合成一个, 因此6阶亮度RGB每两阶中间可以产生3个中间色, 共额外产生5*3+6=21阶色
 */
int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_initmalloc(void);
char *api_malloc(int size);
int api_getkey(int mode);
void api_end(void);

unsigned char rgb2pal(int r, int g, int b, int x, int y);

void HariMain(void) {
    // 分配内存
    api_initmalloc(); // app内存控制器
    char *buf = api_malloc(144 * 164); // 分配内存
    // 打开窗口并返回窗口图层地址
    int win = api_openwin(buf, 144, 164, -1, "color2");
    // 遍历128*128的显示区域像素点, 依次改变其颜色
    int x, y;
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            // 通过四像素合一, 让6亮度产生21亮度效果
            buf[(x + 8) + (y + 28) * 144] = rgb2pal(x * 2, y * 2, 0, x, y); // 共128个像素点要显示256亮度, 因此*2
        }
    }
    // 刷新窗口图层
    api_refreshwin(win, 8, 28, 136, 156);
    // 等待键盘按下任何按键退出
    api_getkey(1);
    // 结束app
    api_end();
}

/*
    通过四像素合一, 让6亮度产生21亮度效果
    - 在四个像素中,降低让其中(0~3)个像素点的亮度减一, 从而生成3个中间亮度
    - r, g, b: 目的亮度
    - x, y: 像素点坐标
*/
unsigned char rgb2pal(int r, int g, int b, int x, int y) {
    // 确定合成的模式, 数字大小代表合成时颜色降阶的像素顺序, 此处降阶顺序为: 左下,右上,右下,左上
    static int table[4] = {3, 1, 0, 2};
    // 判断当前像素属于4像素合成的哪个像素点
    x &= 1; // 只保留奇偶
    y &= 1;
    int i = table[x + y * 2];
    // 计算在21阶亮度(0~20)中属于哪一阶
    r = (r * 21) / 256;
    g = (g * 21) / 256;
    b = (b * 21) / 256;
    // 计算在6阶亮度(0~5)中属于哪一阶
    r = (r + i) / 4;
    g = (g + i) / 4;
    b = (b + i) / 4;
    return 16 + r + g * 6 + b * 36;
}
