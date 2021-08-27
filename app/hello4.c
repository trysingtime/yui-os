/*
    该app带有数据段, 测试此时api调用
*/
void api_putstr0(char *s);
void api_end(void);

void HariMain(void) {
    api_putstr0("hello, world\n");
    api_end();
}
