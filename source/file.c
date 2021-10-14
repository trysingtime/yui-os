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
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
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

/*
    读取文件并解压缩到指定地址
    - clustno: 文件信息中的簇号
    - psize: 保存文件大小的地址(注意该地址的值将被函数所改变)
    - fat: 解压缩后的FAT信息存储地址
*/
char *file_load_compressfile(int clustno, int *psize, int *fat) {
    struct MEMMNG *mng = (struct MEMMNG *) MEMMNG_ADDR; // 内存控制器
    // 读取文件
    char *buf = (char *) memory_alloc_4k(mng, *psize);
    file_loadfile(clustno, *psize, buf, fat, (char *)(ADR_DISKIMG + 0x003e00));
    // 判断是否是压缩文件
    if (*psize >= 17) {
        int teksize = tek_getsize(buf);
        if (teksize > 0) {
            /* 使用tek格式压缩的文件 */
            // 从buf解压缩到tekbuf
            char *tekbuf = (char *) memory_alloc_4k(mng, teksize);
            tek_decomp(buf, tekbuf, teksize);
            // 释放buf
            memory_free_4k(mng, (int)buf, *psize);
            // 重定向buf
            buf = tekbuf;
            // 修改文件大小为解压缩后的大小(psize应使用新变量传入该函数)
            *psize = teksize;
        }
    }
    return buf;
}

/*
    根据文件全名查找磁盘文件信息中的文件
    - filefullname: 文件全名(文件名+'.'+文件后缀), 不区分大小写
    - fileinfo: 磁盘文件信息地址(0x2600~0x4200)
    - max: 磁盘文件信息数量(0x2600~0x4200只能有224个文件信息(32字节))
*/
struct FILEINFO *file_search(char *filefullname, struct FILEINFO *fileinfo, int max) {
    // 清空变量, 用以存放文件名
    char s[12];
    int y;
    for (y = 0; y < 11; y++) {
        s[y] = ' ';
    }
    y = 0;
    // 截取文件名(type 文件名+'.'+文件后缀)(文件名大于8字节无法处理)
    int x;
    for (x = 0; filefullname[x] != 0; x++) {
        if (y >= 11) {
            // 文件名过长
            return 0;
        }
        if (filefullname[x] == '.' && y <= 8) {
            // 读取到'.'则认为文件名已取完
            y = 8;
        } else {
            // 文件名转为大写
            s[y] = filefullname[x];
            if ('a' <= s[y] && s[y] <= 'z') {
                s[y] -= 0x20;
            }
            y++;
        }
    }
    // 根据文件名查找文件
    // 遍历所有文件信息(0x2600~0x4200只能有224个文件信息(32字节))
    for (x = 0; x < max; x++) {
        if (fileinfo[x].name[0] == 0x00) {
            // 文件名第一个字节为0x00代表这一段不包含任何文件名信息
            break;
        }
        if (fileinfo[x].name[0] == 0xe5) {
            // 文件名第一个字节为0xe代表这个文件已被删除
            break;
        }
        // type(文件属性): 一般0x20/0x00,0x01(只读文件),0x02(隐藏文件),0x04(系统文件),0x08(非文件信息,如磁盘名称),0x10目录
        if ((fileinfo[x].type & 0x18) == 0) {
            for (y = 0; y < 11; y++) {
                if (fileinfo[x].name[y] != s[y]) {
                    // 文件名不一致, 查找下一个文件
                    break;
                }
            }
            // 找到文件
            if (y == 11) {
                return fileinfo + x;
            }
        }
    }
    // 没有找到文件
    return 0;
}
