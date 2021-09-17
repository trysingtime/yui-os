int api_getkey(int mode);
int api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_end(void);
void api_beep(int tone);

void HariMain(void) {
    // 初始化定时器
    int timer = api_alloctimer();
    api_inittimer(timer, 128); // 倒计时结束发送128到fifo
    // 每0.01秒频率降低1%
    int i;
    for (i = 20000000; i >= 20000; i -= i / 100) {
        /* 20KHz~20Hz, 人类能听到的范围 */
        // 启动蜂鸣器
        api_beep(i);
        api_settimer(timer, 1); // 0.01s
        // 休眠等待中断(参数为1: 休眠直到中断输入, 0: 不休眠返回-1)
        if (api_getkey(1) != 128) {
            // 按下任意键结束app
            break;
        }
    }
    // 关闭蜂鸣器
    api_beep(0);
    // 结束app
    api_end();
}
