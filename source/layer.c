#include "bootpack.h"

#define LAYER_USED      1 // 图层已使用标识1

/*
    初始化图层管理
    为图层管理分配内存, 将BOOTINFO里的vram地址和画面大小缓存到图层管理, 图层高度默认-1, 图层已使用标识默认0(未使用)
*/
struct LAYERCTL *layerctl_init(struct MEMMNG *memmng, unsigned char *vram, int xsize, int ysize) {
    struct LAYERCTL *ctl;
    int i;
    ctl = (struct LAYERCTL *)memory_alloc_4k(memmng, sizeof(struct LAYERCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (i = 0; i < MAX_LAYERS; i++) {
        ctl->layer[i].flags = 0; // 标志为未使用
        ctl->layer[i].ctl = ctl; // 将图层管理绑定到每个图层
    }
err:
    return ctl;
}

/*
    从图层管理中获取未使用的图层
*/
struct LAYER *layer_alloc(struct LAYERCTL * ctl) {
    struct LAYER *layer;
    int i;
    for (i = 0; i < MAX_LAYERS; i++) {
        if (ctl->layer[i].flags == 0) {
            layer = &ctl->layer[i];
            layer->flags = LAYER_USED; // 标志从未使用改为正在使用
            layer->height = -1;
            return layer;
        }
    }
    return 0;
}

/*
    初始化图层
    layer: 图层地址
    buf: 图层内容地址
    xsize, ysize: 图层大小
    col_inv: 图层颜色和透明度
*/
void layer_init(struct LAYER *layer, unsigned char *buf, int xsize, int ysize, int col_inv) {
    layer->buf = buf;
    layer->bxsize = xsize;
    layer->bysize = ysize;
    layer->col_inv = col_inv;
    return;
}

/*
    根据图层初始位置的相对坐标刷新矩形范围图层
    ctl: 图层管理
    layer: 基于哪个图层的初始位置
    bx0~by0, bx1~by1: 相对于图层初始位置的矩形范围坐标
*/
void layer_refresh(struct LAYER *layer, int bx0, int by0, int bx1, int by1) {
    if (layer->height >= 0) {
        layer_refresh_abs(layer->ctl, layer->vx0 + bx0, layer->vy0 + by0, layer->vx0 + bx1, layer->vy0 + by1, layer->height);
    }
    return;
}

/*
    根据绝对坐标刷新矩形范围图层
    ctl: 图层管理
    vx0~vx1, vy0~vy1: 指定矩形范围坐标
    h0: 刷新大于此层的图层
*/
void layer_refresh_abs(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram;
    struct LAYER *layer;
    // 如果范围超出画面则修正
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    // 从图层h0层开始升序绘制
    for (h = h0; h <= ctl->top; h++) {
        layer = ctl->layersorted[h];
        buf = layer->buf;

        // 将绝对坐标转换为相对每个图层起始位置的相对坐标
        bx0 = vx0 - layer->vx0;
        by0 = vy0 - layer->vy0;
        bx1 = vx1 - layer->vx0;
        by1 = vy1 - layer->vy0;
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > layer->bxsize) { bx1 = layer->bxsize; }
        if (by1 > layer->bysize) { by1 = layer->bysize; }

        // 根据图层相对坐标一个个像素点绘制
        for (by = by0; by < by1; by++) {
            vy = layer->vy0 + by; // 确定当前要绘制的y轴点
            for (bx = bx0; bx < bx1; bx++) {
                vx = layer->vx0 + bx; // 确定当前要绘制的x轴点
                if (vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1) {
                    // 只刷新指定范围vx0<=vx<vx1, vy0<=vy<vy1
                    c = buf[by * layer->bxsize + bx];  // 确定当前要绘制的颜色
                    if (c != layer->col_inv) {
                        // 图层颜色和内容颜色不一致时才绘制, 也就是说图层颜色和内容颜色一致时透明
                        vram[vy * ctl->xsize + vx] = c;
                    }
                }
            }
        }
    }
    return;
}

/*
    改变图层高度并刷新图层
    需要根据高度升序重建索引: layersorted
*/
void layer_updown(struct LAYER *layer, int height) {
    struct LAYERCTL *ctl = layer->ctl;
    int h, old = layer->height; // 保存修改前图层高度

    // 如果指定高度过高或高低, 则进行修正
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    layer->height = height;

    // 下面重建索引layersorted
    if (old > height) {
        if (height >= 0) {
            // 图层降低
            for (h = old; h > height; h--) {
                ctl->layersorted[h] = ctl->layersorted[h - 1];
                ctl->layersorted[h]->height = h;
            }
            ctl->layersorted[height] = layer;
        } else {
            // 图层隐藏
            for (h = old; h < ctl->top; h++) {
                ctl->layersorted[h] = ctl->layersorted[h + 1];
                ctl->layersorted[h]->height = h;
            }
            ctl->top--;
        }
    } else {
        if (old >= 0) {
            // 图层升高
            for (h = old; h < height; h++) {
                ctl->layersorted[h] = ctl->layersorted[h + 1];
                ctl->layersorted[h]->height = h;
            }
            ctl->layersorted[height] = layer;
        } else {
            // 图层浮现
            for (h = ctl->top; h >= height; h--) {
                ctl->layersorted[h + 1] = ctl->layersorted[h];
                ctl->layersorted[h + 1]->height = h + 1;
            }
            ctl->layersorted[height] = layer;
            ctl->top++;
        }
    }
    layer_refresh_abs(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, height); // 改变图层高度, 刷新整个图层
    return;
}

/*
    改变图层坐标并刷新图层
*/
void layer_slide(struct LAYER *layer, int vx0, int vy0) {
    int old_vx0 = layer->vx0, old_vy0 = layer->vy0;
    layer->vx0 = vx0;
    layer->vy0 = vy0;
    if (layer->height >=0) {
        // 图层正在显示则需刷新图层
        layer_refresh_abs(layer->ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize, 0); // 移动后, 原有位置所有图层需要刷新
        layer_refresh_abs(layer->ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize, layer->height); // 移动后, 目标位置只需刷新大于该图层高度的图层
    }
    return;
}

/*
    释放已使用图层
    先隐层图层, 再修改已使用标识为0(未使用)
*/
void layer_free(struct LAYER *layer) {
    // 隐藏图层
    if (layer->height >= 0) {
        layer_updown(layer, -1);
    }
    // 修改已使用标识为0(未使用)
    layer->flags = 0;
    return;
}
