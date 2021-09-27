/* 打开文件"ipl10.nas"并打印内容 */
#include "../../api/api.h"

void HariMain(void) {
    // 打开文件"ipl10.nas"
    int fh = api_fopen("ipl10.nas");
    if (fh != 0) {
        char c;
        // 遍历读取每一个字节并显示
        for (;;) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
        }
    }
    api_end();
}
