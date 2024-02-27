/* 打开一个文件并打印内容 */
#include "../../api/api.h"

void HariMain(void) {
    // 获取控制台指令
    char cmdline[30];
    api_cmdline(cmdline, 30);
    char *p;
    // 跳过指令中第一个空格前的内容
    for (p = cmdline; *p > ' '; p++) { }
    // 跳过指令中的第一个空格
    for (; *p == ' '; p++) { }
    // 打开文件
    int fh = api_fopen(p);
    if (fh != 0) {
        char c;
        // 遍历读取每一个字节并显示
        for (;;) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
        }
    } else {
        api_putstr0("File not found.\n");
    }
    api_end();
}
