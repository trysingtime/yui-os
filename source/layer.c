#include "bootpack.h"

#define LAYER_USED      1 // 图层已使用标识1

/*
    初始化图层控制器
    为图层控制器分配内存, 将BOOTINFO里的vram地址和画面大小缓存到图层控制器中, 图层高度默认-1, 图层已使用标识默认0(未使用)
    - memmng: 内存控制器
    - vram, xsize, ysize: BOOTINFO里的vram地址和画面大小
*/
struct LAYERCTL *layerctl_init(struct MEMMNG *memmng, unsigned char *vram, int xsize, int ysize) {
    struct LAYERCTL *ctl;
    int i;
    // 为图层控制器分配内存
    ctl = (struct LAYERCTL *)memory_alloc_4k(memmng, sizeof(struct LAYERCTL));
    if (ctl == 0) {
        goto err;
    }
    // 为每个像素点分配内存
    ctl->map = (unsigned char *) memory_alloc_4k(memmng, xsize * ysize);
    if (ctl->map == 0) {
        memory_free_4k(memmng, (int) ctl, sizeof(struct LAYERCTL));
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (i = 0; i < MAX_LAYERS; i++) {
        ctl->layers[i].flags = 0; // 标志为未使用
        ctl->layers[i].ctl = ctl; // 将图层控制器绑定到每个图层
    }
err:
    return ctl;
}

/*
    从图层控制器中获取未使用的图层
    - ctl: 图层控制器
*/
struct LAYER *layer_alloc(struct LAYERCTL * ctl) {
    struct LAYER *layer;
    int i;
    for (i = 0; i < MAX_LAYERS; i++) {
        if (ctl->layers[i].flags == 0) {
            layer = &ctl->layers[i];
            layer->flags = LAYER_USED; // 标志从未使用改为正在使用
            layer->height = -1;
            layer->task = 0; // 默认图层不属于任何task
            return layer;
        }
    }
    return 0;
}

/*
    初始化图层
    - layer: 图层地址
    - buf: 图层内容地址
    - xsize, ysize: 图层大小
    - col_inv: 图层颜色和透明度
*/
void layer_init(struct LAYER *layer, unsigned char *buf, int xsize, int ysize, int col_inv) {
    layer->buf = buf;
    layer->bxsize = xsize;
    layer->bysize = ysize;
    layer->col_inv = col_inv;
    return;
}

/*
    根据相对坐标刷新指定图层(矩形范围)
    - ctl: 图层控制器
    - layer: 基于哪个图层的初始位置
    - bx0~by0, bx1~by1: 相对于图层初始位置的矩形范围坐标
*/
void layer_refresh(struct LAYER *layer, int bx0, int by0, int bx1, int by1) {
    if (layer->height >= 0) {
        layer_refresh_abs(layer->ctl, layer->vx0 + bx0, layer->vy0 + by0, layer->vx0 + bx1, layer->vy0 + by1, layer->height, layer->height);
    }
    return;
}

/*
    根据绝对坐标刷新矩形范围内所有图层
    - ctl: 图层控制器
    - vx0~vx1, vy0~vy1: 指定矩形范围坐标
    - h0~h1: 刷新指定高度范围的图层
*/
void layer_refresh_abs(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, sid, *vram = ctl->vram, *map = ctl->map;
    struct LAYER *layer;
    // 如果范围超出画面则修正
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    // 从图层h0层开始升序绘制
    for (h = h0; h <= h1; h++) {
        layer = ctl->layersorted[h];
        buf = layer->buf;
        sid = layer - ctl->layers; // 计算是图层组layers[i]中的第几组

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
                if (map[vy * ctl->xsize + vx] == sid) {
                    // map(像素点)归该图层负责才刷新
                    c = buf[by * layer->bxsize + bx];  // 确定当前要绘制的颜色
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}

/*
    根据绝对坐标刷新map(像素点)所负责的图层(矩形范围)
    图层map: 将屏幕每个像素点映射成map, 在map上记录该像素点由哪一图层(当前图层高度最大的层)负责显示
    - ctl: 图层控制器
    - vx0~vx1, vy0~vy1: 指定矩形范围坐标
    - h0: 刷新大于此层的图层
*/
void layer_refresh_map(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, sid, *map = ctl->map;
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
        sid = layer - ctl->layers; // 计算是图层组layers[i]中的第几组

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
                        map[vy * ctl->xsize + vx] = sid; // 指定map(像素点)由图层sid负责
                    }
                }
            }
        }
    }
    return;
}

/*
    改变图层高度并刷新图层
    - 实际图层高度由已有的图层决定, 而不是传入的值, 相同高度后者更低
    需要根据高度升序重建索引: layersorted
    图层降低: 重绘上层(大于)图层~顶部图层
    图层隐藏: 重绘上层(大于)图层~顶部图层(-1)
    图层上升: 重绘本图层
    - layer: 指定图层
    - height: 指定高度(实际高度由已有图层决定, 而不是该值)
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
            layer_refresh_map(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, height + 1); // 重新计算像素点由哪个图层负责
            layer_refresh_abs(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, height + 1, old); // 重绘上层图层
        } else {
            // 图层隐藏
            for (h = old; h < ctl->top; h++) {
                ctl->layersorted[h] = ctl->layersorted[h + 1];
                ctl->layersorted[h]->height = h;
            }
            ctl->top--;
            layer_refresh_map(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, 0); // 重新计算像素点由哪个图层负责
            layer_refresh_abs(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, 0, old - 1); // 重绘上层图层
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
        layer_refresh_map(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, height); // 重新计算像素点由哪个图层负责
        layer_refresh_abs(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, height, height); // 重绘该图层
    }
    return;
}

/*
    改变图层坐标并刷新图层
    - layer: 指定图层
    - vx0, vy0: 目的坐标
*/
void layer_slide(struct LAYER *layer, int vx0, int vy0) {
    int old_vx0 = layer->vx0, old_vy0 = layer->vy0;
    layer->vx0 = vx0;
    layer->vy0 = vy0;
    if (layer->height >=0) {
        // 图层正在显示则需刷新图层
        layer_refresh_map(layer->ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize, 0); // 移动后, 原有位置像素点需要重新计算由哪个图层负责
        layer_refresh_map(layer->ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize, layer->height); // 移动后, 目标位置像素点需要重新计算由哪个图层负责
        layer_refresh_abs(layer->ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize, 0, layer->height - 1); // 原有位置图层刷新, 只需重绘下层图层
        layer_refresh_abs(layer->ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize, layer->height, layer->height); //目标位置图层刷新, 只需重绘该图层, 若像素点不是该图层负责, 则相应像素点不会刷新
    }
    return;
}

/*
    释放已使用图层
    先隐层图层, 再修改已使用标识为0(未使用)
    - layer: 指定图层
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
