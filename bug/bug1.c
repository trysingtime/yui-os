void api_putchar(int c);
void api_end(void);

/*
    栈异常bug
*/
void HariMain(void) {
        /*
        异常: 超出数组边界, 应产生栈异常
        未注册栈异常前: 模拟器没有报错, 真机直接重启
        注册栈异常后: 模拟器没有报错, 真机对字符'B'正常打印, 字符'C'才产生栈异常. 因为a[102]虽然超出数组边界, 但没有超出app数据段边界
        总结: 栈异常只保护操作系统, 禁止app访问自身数据段以外的内存地址, 对数据段内的数据bug不处理
    */
    char a[100];
    a[10] = 'A';
    api_putchar(a[10]);
    a[102] = 'B';
    api_putchar(a[102]);
    a[123] = 'C';
    api_putchar(a[123]);
    api_end();
}
