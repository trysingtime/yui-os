/*
    需要操作系统强制结束的app
*/
void app_putchar(int c);
void api_end(void);

void HariMain(void) {
    for(;;) {
        api_putchar('a');
    }
}
