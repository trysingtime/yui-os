/* 打印10000以内的质数(只能被1及其本身整除的大于1的自然数) */
#include <stdio.h>
#include "../../api/api.h"

#define MAX     10000

void HariMain(void) {
    api_initmalloc();
    char *flag = api_malloc(MAX); // 不使用栈, 因此超过4KB也不会调用_alloca函数
    int i, j;
    for (i = 0; i < MAX; i++) {
        flag[i] = 0;
    }
    for (i = 2; i < MAX; i++) {
        // 没有标志的代表是质数, 打印出来
        if (flag[i] == 0) {
            char s[8];
            sprintf(s, "%d ", i);
            api_putstr0(s);
            // 标志能被其他数整除的自然数
            for (j = i * 2; j < MAX; j += i) {
                flag[j] = 1;
            }
        }
    }
    api_end();
}
