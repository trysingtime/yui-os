#include "bootpack.h"

/*
    绘制窗口
    - buf: 窗体内容起始地址
    - xsize, ysize: 窗体大小
    - titile: 窗体标题
    - activa: 该窗口是否激活
*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char active) {
    /*
        边界3像素 = 外阴影1像素 + 内阴影1像素 + 边界1像素
        内容y轴起始21 = 边界3像素 + 标题栏高度18像素 - 1
    */
	boxfill8(buf, xsize, COL8_C6C6C6,           0,         0,         xsize - 1, 0        ); // 上边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF,           1,         1,         xsize - 2, 1        ); // 上边界-内阴影-白
	boxfill8(buf, xsize, COL8_C6C6C6,           0,         0,         0,         ysize - 1); // 左边界-亮灰
	boxfill8(buf, xsize, COL8_FFFFFF,           1,         1,         1,         ysize - 2); // 左边界-内阴影-白
	boxfill8(buf, xsize, COL8_848484,           xsize - 2, 1,         xsize - 2, ysize - 2); // 右边界-暗灰
	boxfill8(buf, xsize, COL8_000000,           xsize - 1, 0,         xsize - 1, ysize - 1); // 右边界-阴影-黑
	boxfill8(buf, xsize, COL8_C6C6C6,           2,         2,         xsize - 3, ysize - 3); // 窗体-亮灰
	boxfill8(buf, xsize, COL8_848484,           1,         ysize - 2, xsize - 2, ysize - 2); // 下边界-暗灰
	boxfill8(buf, xsize, COL8_000000,           0,         ysize - 1, xsize - 1, ysize - 1); // 下边界-阴影-黑
    // 绘制窗口标题栏
    make_title8(buf, xsize, title, active);
	return;
}

/*
    绘制窗口标题栏
    - buf: 窗体内容起始地址
    - xsize: 窗体宽度
    - titile: 窗体标题
    - activa: 该窗口是否激活
*/
void make_title8(unsigned char *buf, int xsize, char *title, char active) {
    // 窗口-标题栏
    /* 若窗口激活则标题栏显示暗青, 否则显示暗灰 */
    char title_color, title_backgroud_color;
    if (active != 0) {
        title_color = COL8_FFFFFF;
        title_backgroud_color = COL8_000084;
    } else {
        title_color = COL8_C6C6C6;
        title_backgroud_color = COL8_848484;
    }
	boxfill8(buf, xsize, title_backgroud_color, 3,         3,         xsize - 4, 20       ); // 标题栏-暗青/暗灰
	putfonts8_asc(buf, xsize, 24, 4, title_color, title); // 标题

    // 窗口-右上角按钮X, 坐标(5, xsize - 21), 大小(14 * 16)
    static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
    return;
}

/*
    切换窗口标题栏颜色
    - 仅切换窗口标题栏字体颜色和背景颜色, 其他颜色不变
    - layer: 指定的窗口图层
    - active: 激活/反激活窗口(0: 反激活, 1: 激活)
*/
void change_title8(struct LAYER *layer, char active) {
    /* 若窗口激活则标题栏从暗灰转为暗青, 否则则从暗青转为暗灰 */
    char old_title_color, old_title_backgroud_color; // 旧颜色
    char new_title_color, new_title_backgroud_color; // 新颜色
    if (active != 0) {
        old_title_color = COL8_C6C6C6;
        old_title_backgroud_color = COL8_848484;        
        new_title_color = COL8_FFFFFF;
        new_title_backgroud_color = COL8_000084;
    } else {
        old_title_color = COL8_FFFFFF;
        old_title_backgroud_color = COL8_000084;        
        new_title_color = COL8_C6C6C6;
        new_title_backgroud_color = COL8_848484;
    }

    // 遍历标题栏所有像素点, 找到所有"旧颜色"替换成"新颜色", 其他颜色不变
    int x, y;
    for (y = 3; y <= 20; y++) {
        for (x = 3; x <= layer->bxsize - 4; x++) {
            int color = layer->buf[y * layer->bxsize + x]; // 原有颜色
            if (x <= layer->bxsize - 22 && color == old_title_color) {
                // 替换标题文字颜色
                color = new_title_color;
            } else if (color == old_title_backgroud_color) {
                // 替换标题背景颜色
                color = new_title_backgroud_color;
            }
            layer->buf[y * layer->bxsize + x] = color;
        }
    }
    // 刷新
    layer_refresh(layer, 3, 3, layer->bxsize, 21);
    return;
}

/*
    切换窗口图层
    - layerctl: 图层控制器
    - current_layer: 当前窗口图层
    - target_layer: 目标窗口图层(若为0则不指定图层, 自动使用下一图层)
*/
void switch_window(struct LAYERCTL *layerctl, struct LAYER *current_layer, struct LAYER *target_layer) {
    // 旧窗口失去焦点
    if (current_layer->flags != 0) {
        // 切换窗口标题栏颜色
        change_title8(current_layer, 0);
        // 关闭光标
        if ((current_layer->flags & 0x20) != 0) {
            /* 当前窗口为"窗口程序-console", 存在光标, 需要关闭 */
            fifo32_put(&current_layer->task->fifo, 3); // 通过中断fifo通知光标关闭
        }
    }
    // 切换图层
    if (target_layer == 0) {
        /* 没有指定目标图层, 自动切换到下一图层 */
        int i = current_layer->height - 1;
        if (i == 0) {
            i = layerctl->top - 1;
        }
        current_layer = layerctl->layersorted[i];
    } else {
        /* 指定了目标图层, 则直接使用 */
        current_layer = target_layer;
    }
    // 新窗口获得焦点
    change_title8(current_layer, 1);
    if ((current_layer->flags & 0x20) != 0) {
        /* 当前窗口为"窗口程序-console", 存在光标, 需要开启 */
        fifo32_put(&current_layer->task->fifo, 2); // 通过中断fifo通知光标开启
    }
    keyboard_input_layer = current_layer;
}

/*
    绘制文本框
    - layer: 指定图层
    - x0, y0: 文本框在图层中的坐标(左边和上边溢出3个像素, 右边和下边溢出2个像素)
    - xsize, ysize: 文本框长度和宽度
    - color: 文本框背景色
*/
void make_textbox8(struct LAYER *layer, int x0, int y0, int xsize, int ysize, int color) {
    int x1 = x0 + xsize, y1 = y0 + ysize;
    boxfill8(layer->buf, layer->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3); // 上边界-暗灰
    boxfill8(layer->buf, layer->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2); // 上边界-内阴影-黑
    boxfill8(layer->buf, layer->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1); // 左边界-亮灰
    boxfill8(layer->buf, layer->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0); // 左边界-内阴影-黑
    boxfill8(layer->buf, layer->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2); // 下边界-白
    boxfill8(layer->buf, layer->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1); // 下边界-内阴影-亮灰
    boxfill8(layer->buf, layer->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2); // 右边界-白
    boxfill8(layer->buf, layer->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1); // 右边界-内阴影-亮灰
    boxfill8(layer->buf, layer->bxsize, color      , x0 - 1, y0 - 1, x1 + 0, y1 + 0); // 文本输入框
    return;
}

/*
    在指定图层绘制字符串
    - layer: 指定图层
    - x,y: 指定图层内部坐标
    - color: 字符串颜色
    - backcolor: 背景颜色
    - string: 字符串地址
    - length: 字符串长度
*/
void putfonts8_asc_layer(struct LAYER *layer, int x, int y, int color, int backcolor, char *string, int length) {
        boxfill8(layer->buf, layer->bxsize, backcolor, x, y, x + length * 8 - 1, y + 15); // 擦除原有数据(绘制背景色矩形遮住之前绘制好的数据)
        struct TASK *task = task_current();
        if (task->langmode != 0 && task->langbuf != 0) {
            /* 全角符号 */
            putfonts8_asc(layer->buf, layer->bxsize, x, y, color, string); // 显示新的数据
            layer_refresh(layer, x - 8, y, x + length * 8, y + 16); // 刷新图层
        } else {
            /* 半角符号 */
            putfonts8_asc(layer->buf, layer->bxsize, x, y, color, string); // 显示新的数据
            layer_refresh(layer, x, y, x + length * 8, y + 16); // 刷新图层
        }
        return;
}
