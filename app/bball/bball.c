/* 绘制直线组成的球 */
#include "../../api/api.h"

void HariMain(void) {
    // 新建窗口
	char buf[216 * 237];
    int win = api_openwin(buf, 216, 237, -1, "bball");
    // 点阵
    struct POINT {
        int x, y;
    };
	static struct POINT table[16] = {
		{ 204, 129 }, { 195,  90 }, { 172,  58 }, { 137,  38 }, {  98,  34 },
		{  61,  46 }, {  31,  73 }, {  15, 110 }, {  15, 148 }, {  31, 185 },
		{  61, 212 }, {  98, 224 }, { 137, 220 }, { 172, 200 }, { 195, 168 },
		{ 204, 129 }
	};
    // 绘制
    api_boxfillwin(win,   8,  29, 207, 228,   0);
    int i, j, dis;
    for (i = 0; i <= 14; i++) {
        for (j = i + 1; j <= 15; j++) {
            dis = j - i;
            if (dis >= 8) {
                dis = 15 - dis;
            }
            if (dis != 0) {
                api_linewin(win, table[i].x, table[i].y, table[j].x, table[j].y, 8 - dis);
            }
        }
    }
    // 等待键盘输入
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            /* 回车键则break */
            break;
        }
    }
    api_end();
}
