#include "bootpack.h"

/*
    FAT(file allocation table, 文件分配表)
    - 软件中位于c0-h0-s2开始的9个扇区中, 相当于地址0x0200~0x13ff, 备份地址0x1400~0x25ff
    - 文件信息包含簇号, 该簇号可以计算文件第一个簇的地址(512字节), 后续地址需要根据簇号去FAT表查询, FAT表的内容代表后续地址所在的簇
    - FAT中的数据需要解压缩, 每3个字节为一组, 如(ab cd ef -> dab efc, 03 40 00 -> 003 004)
    未解压缩前  : 0x0200: F0 FF FF 03 40 00 05 60 00 70 80 00 09 A0 00 0B
    解压缩后    : 0x0200: FF0  FFF 003  004 005  006 007  008 009  00A
    根据簇号2(2*512+0x3e00=0x4200~0x43ff)查询FAT表获取下一簇号, 值为003, 因此后续地址为簇号3地址(3*512+0x3e00=0x4400~0x45ff);
    再次簇号3查询FAT表获取下一簇号, 值为004, 依次类推, 直到遇到值为FF8~FFF之一结束
*/
/*
    将FAT信息读取并解压缩到内存(int类型)
    - fat: 解压缩后存放的目的地址(int类型)
    - img: FAT地址(char类型)
*/
void file_readfat(int *fat, unsigned char *img) {
    int i, j = 0;
    // 遍历所有FAT(每个扇区都对应一个FAT, 一张软盘共有2880个扇区)
    for (i = 0; i < 2880; i += 2) {
        // FAT中的数据需要解压缩, 每3个字节为一组, 此处3个字节解压缩为两个int, 如(ab cd ef -> dab efc, 03 40 00 -> 003 004)
        fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff; // 左移运算结果为int类型, ab cd -> cd00 ab -> dab
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff; // 左移运算结果为int类型, cd ef -> 0e00 fc -> efc
        j += 3; // 3个字节一组解压缩
    }
    return;
}

/*
    读取文件内容到指定地址
    - clustno: 文件信息中的簇号
    - size: 文件信息中的文件大小
    - buf: 存储文件内容的目的地址
    - fat: 解压缩后的FAT信息存储地址
    - img: 簇号偏移地址(文件内容起始地址 = 簇号偏移地址+簇号*512)
*/
void fiel_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
    int i;
    for(;;) {
        /* 文件大小<=512字节, 则无需查询FAT, 直接通过簇号偏移地址计算 */
        if (size <= 512) {
            for (i = 0; i < size; i++) {
                // 文件内容起始地址 = 簇号偏移地址+簇号*512 = img[clustno * 512]
                // 此处从起始地址开始一个个字节读取到buf
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        /* 文件大小>512字节, 需查询FAT */
        // 先根据簇号直接获取起始512字节内容
        for (i = 0; i < 512; i++) {
            // 文件内容起始地址 = 簇号偏移地址+簇号*512 = img[clustno * 512]
            // 此处从起始地址开始一个个字节读取到buf
            buf[i] = img[clustno * 512 + i];
        }
        // 超出512字节部分需要查询FAT, 获得下一簇号, 再根据簇号计算内容地址, 循环直到读取完毕
        size -= 512;
        buf += 512;
        clustno = fat[clustno]; // 根据当前簇号获取下一簇号
    }
    return;
}
