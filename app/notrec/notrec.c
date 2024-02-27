/* 绘制非矩形窗口 */
#include "../../api/api.h"

void HariMain(void) {
	char buf[150 * 70];
    int win = api_openwin(buf, 150, 70, 255/*255作为透明色*/, "notrec");
    api_boxfillwin(win,   0,  50,  34,  69, 255);
    api_boxfillwin(win, 115,  50, 149,  69, 255);
    api_boxfillwin(win,  50,  30,  99,  49, 255);
    // 等待键盘输入
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            /* 回车键则break */
            break;
        }
    }
    api_end();
}
