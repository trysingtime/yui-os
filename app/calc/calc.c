/* 计算器 */
#include <stdio.h>
#include "../../api/api.h"

#define INVALID -0x7fffffff

/*
    字符串转整数
    - s: 字符串地址
    - endp: 转换字符串时所读取到的字符串末尾地址的地址(一般为0)
    - base: 进制(0代表自动识别, 0x开头识别为十六进制, 0开头识别为八进制)
*/
int strtol(char *s, char **endp, int base); /* 标准函数(stdlib.h) */
char *skipspace(char *p);
int getnum(char **pp, int priority);

void HariMain(void) {
    // 获取输入的指令
    char s[30];
    api_cmdline(s, 30);
    // 跳到第一个空格
    char *p;
    for (p = s; *p > ' '; p++) {}
    // 计算结果
    int i = getnum(&p, 9);
    // 显示结果
    if (i == INVALID) {
        api_putstr0("error!\n");
    } else {
        sprintf(s, "= %d = 0x%x\n", i, i);
        api_putstr0(s);
    }
    api_end();
}

/*
    跳过字符串最前面的空格
    - p: 字符串地址
*/
char *skipspace(char *p) {
    for (; *p == ' '; p++) {}
    return p;
}

/*
    计算结果
*/
int getnum(char **pp, int priority) {
    int i = INVALID;
    int j;
    char *p = *pp;
    p = skipspace(p);

    // 单项运算符
    if (*p == '+') {
        /* 原值 */
        p = skipspace(p + 1);
        i = getnum(&p, 0);
    } else if (*p == '-') {
        /* 负数 */
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = -i;
        }
    } else if (*p == '~') {
        /* 取反 */
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = ~i;
        }
    } else if (*p == '(') {
        /* 括号 */
        p = skipspace(p + 1);
        i = getnum(&p, 9);
        if (*p == ')') {
            p = skipspace(p + 1);
        } else {
            i = INVALID;
        }
    } else if ('0' <= *p && *p <= '9') {
        // 字符串转整数
        i = strtol(p, &p, 0);
    } else {
        i = INVALID;
    }

    // 二项运算符
    for (;;) {
        if (i == INVALID) {
            break;
        }
        p = skipspace(p);
        if (*p == '+' && priority > 2) {
            /* 加 */
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i += j;
            } else {
                i = INVALID;
            }
        } else if (*p == '-' && priority > 2) {
            /* 减 */
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i -= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '*' && priority > 1) {
            /* 乘 */
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID) {
                i *= j;
            } else {
                i = INVALID;
            } 
        } else if (*p == '/' && priority > 1) {
            /* 除 */
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i /= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '%' && priority > 1) {
            /* 取余 */
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i %= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '<' && p[1] == '<' && priority > 3) {
            /* 左位移 */
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i <<= j;
            } else {
                i = INVALID;
            }    
        } else if (*p == '>' && p[1] == '>' && priority > 3) {
            /* 右位移 */
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i >>= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '&' && priority > 4) {
            /* 与 */
            p = skipspace(p + 1);
            j = getnum(&p, 4);
            if (j != INVALID) {
                i &= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '^' && priority > 5) {
            /* 异或 */
            p = skipspace(p + 1);
            j = getnum(&p, 5);
            if (j != INVALID) {
                i ^= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '|' && priority > 6) {
            /* 或 */
            p = skipspace(p + 1);
            j = getnum(&p, 6);
            if (j != INVALID) {
                i |= j;
            } else {
                i = INVALID;
            }                                                                                     
        } else {
            break;
        }
    }
    
    p = skipspace(p);
    *pp = p;
    return i;
}
