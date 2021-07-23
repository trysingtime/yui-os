#include "bootpack.h"


#define EFLAGS_AC_BIT           0x00040000 /* 用于检查EFLAGS寄存器(32位): 此处第18位AC_BIT,进位标志(第0位),中断标志(第9位)*/
#define CR0_CACHE_DISABLE       0x60000000 /* CR0寄存器(32位): 此处bit30+bit29置1禁止缓存,bit31置为0禁用分页,bit0置为1切换到保护模式*/

/*
    禁用CPU(486及以上)缓存
    内存容量检查前需禁用CPU缓存, 禁用CPU缓冲需先判断CPU是否存在缓存, 
    486CPU及以上才存在缓冲, 可以通过EFLAGS寄存器(32位)的AC_BIT(18bit)判断CPU
*/
unsigned int memtest(unsigned int start, unsigned int end) {
    char flag486 = 0;
    unsigned int eflag, cr0, i;

    // 确认CPU是386还是486以上, EFLAGS寄存器(32位)的AC_BIT(18bit)判断
    eflag = io_load_eflags();
    // AC_BIT置为1
    eflag |= EFLAGS_AC_BIT; // 如果是386, 即使设定AC=1, AC的值还会自动回到0
    io_store_eflags(eflag);
    // 判断当前AC_BIT
    eflag = io_load_eflags();
    if ((eflag & EFLAGS_AC_BIT) != 0) {
        flag486 = 1;
    }
    // 还原AC_BIT
    eflag &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflag);
    // 禁止缓存
    if (flag486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; // CR0寄存器(32位),bit30+bit29置1禁止缓存,bit31置为0禁用分页,bit0置为1切换到保护模式
        store_cr0(cr0);
    }
    // 内存容量检查
    i = memtest_sub(start, end);
    // 还原(允许缓存)
    if (flag486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    return i;
}

/*
    内存容量检查(编译器会优化此方法, 导致此方法无效, 使用汇编方法memtest_sub替代)
*/
unsigned int memtest_sub_c(unsigned int start, unsigned int end) {
    /*
    步骤:
    1. 获取需要检查的内存地址           *p
    2. 保存该地址原值                   old=*p
    3. 写入一个准备好的值               *p=pat0
    4. 反转写入的值                     *p ^= 0xffffffff
    5. 检查反转结果与准备好的反转值     if (*p != pat1)
    6. 再次反转值                       *p ^= 0xffffffff
    7. 检查值是否恢复                   if (*p != pat0)
    8. 还原原值                         *p=old
    9. 获取下一个需要检查的内存地址     p += 4(4Byte)或者 p += 0x1000(4KB)或者其他
    */
   
   unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
   for (i = start; i <= end; i += 0x1000) {   // 9. 获取下一个需要检查的内存地址
       p = (unsigned int *) (i + 0xffc);    // 1. 获取需要检查的内存地址(只检查末尾4字节)
       old = *p;                            // 2. 保存该地址原值
       *p = pat0;                           // 3. 写入一个准备好的值
       *p ^= 0xffffffff;                    // 4. 反转写入的值
       if (*p != pat1) {                    // 5. 检查反转结果与准备好的反转值
not_memory:
            *p = old;
            break;           
       }
       *p ^= 0xffffffff;                    // 6. 再次反转值
       if (*p != pat0) {                    // 7. 检查值是否恢复
           goto not_memory;
       }
       *p = old;                            // 8. 还原原值
   }
   return i;
}

/*
    初始化"内存空闲信息-汇总"
*/
void memmng_init(struct MEMMNG *mng) {
    mng->rows = 0;
    mng->maxrows = 0;
    mng->lostsize = 0;
    mng->lostrows = 0;
    return;
}

/*
    空闲内存总和
*/
unsigned int free_memory_total(struct MEMMNG *mng) {
    unsigned int i, total = 0;
    for (i = 0; i < mng->rows; i++) {
        total += mng->freeinfo[i].size;
    }
    return total;
}

/*
    分配指定大小的内存
    遍历内存空闲信息, 找出第一条空闲内存大于所需内存的记录, 将内存空闲记录的起始空间分配出去, 剩余空间重新记录,
    若剩余空间为0, 则删除该记录, 后续记录依次前移
*/
unsigned int memory_alloc(struct MEMMNG *mng, unsigned int size) {
    unsigned int i, a;
    // 遍历内存空闲信息
    for (i = 0; i < mng->rows; i++) {
        if (mng->freeinfo[i].size >= size) {
            // 找出第一条空闲内存大于所需内存的记录, 将内存空闲记录的起始空间分配出去, 剩余空间重新记录
            a = mng->freeinfo[i].addr;
            mng->freeinfo[i].addr += size;
            mng->freeinfo[i].size -= size;
            if (mng->freeinfo[i].size == 0) {
                // 若剩余空间为0, 则删除该记录, 后续记录依次前移
                mng->rows--;
                for (; i < mng->rows; i++) {
                    mng->freeinfo[i] = mng->freeinfo[i + 1];
                }
            }
        return a;
        }
    }
    return 0;
}

/*
    释放指定起始地址和大小的内存
    新增一条内存空闲信息记录, 若释放的内存段能与前后内存空闲信息段相连, 则添加到那些记录上, 无需新增记录
*/
int memory_free(struct MEMMNG *mng, unsigned int addr, unsigned int size) {
    int i, j;

    // 先判断需要释放的内存起始地址在哪一条内存空闲信息中
    for (i = 0; i < mng->rows; i++) {
        if (mng->freeinfo[i].addr > addr) {
            // freeinfo[i - 1].addr < addr < freeinfo[i].addr
            break;
        }
    }
    if (i > 0) {
        // 前一个空闲内存段刚好可以和释放的内存段连在一起
        if (mng->freeinfo[i - 1].addr + mng->freeinfo[i - 1].size == addr) {
            mng->freeinfo[i - 1].size += size;
            // 后一个空闲内存段刚好也可以和释放的内存段连在一起
            if (i < mng->rows && addr + size == mng->freeinfo[i].addr) {
                mng->freeinfo[i-1].size += mng->freeinfo[i].size;
                // 删除空闲内存信息, 后续记录依次前移
                mng->rows--;
                for (; i < mng->rows; i++) {
                    mng->freeinfo[i] = mng->freeinfo[i + 1];
                }
            }
            return 0;
        }
    }
    // 仅后一个空闲内存段可以和释放的内存段连在一起
    if (i < mng->rows && addr + size == mng->freeinfo[i].addr) {
        mng->freeinfo[i].addr = addr;
        mng->freeinfo[i].size += size;
        return 0;
    }
    // 空闲内存段和释放内存不能相连的情况
    if (mng->rows < MEMMNG_SIZE) {
        mng->rows++;
        if (mng->maxrows < mng->rows) {
            mng->maxrows = mng->rows;
        }
        // freeinfo[i]要记录新释放的内存段, 因此之后的记录需先依次后移
        for (j = mng->rows - 1; j > i; j--) {
            mng->freeinfo[j] = mng->freeinfo[j - 1];
        }
        mng->freeinfo[i].addr = addr;
        mng->freeinfo[i].size = size;
        return 0;
    }
    // 内存空闲信息记录数超过上限
    mng->lostrows++;
    mng->lostsize += size;
    return -1;
}

/*
    分配指定大小的内存(最小单位4KB)
    memory_alloc()以1字节为单位, 内存划分的过于细碎, 很容易超出记录上限
*/
unsigned int memory_alloc_4k(struct MEMMNG *mng, unsigned int size) {
    unsigned int a;
    /*
    二进制/十六进制
        向下取整(0x1000为单位): i & 0xfffff000, 例如 0x12345678 & 0xfffff000 = 0x12345000
        向上取整(0x1000为单位): if ((i & 0xfff) != 0) {i = i & 0xfffff000 + 0x1000}, if语言排除类似0x12345000这种刚好符合取整的情况
        向上取整(0x1000为单位)2: i = (i + 0xfff) & 0xfffff000, 加上0xfff(0x1000 - 1)后向下取整
    十进制
        向下取整: i/100*100
    使用与运算比除法运算快很多, 因此内存管理单位以二进制为主, 例如0x1000(4KB)
    */
    size = (size + 0xfff) & 0xfffff000;
    a = memory_alloc(mng, size);
    return a;
}

/*
    释放指定起始地址和大小的内存(最小单位4KB)
    memory_free()以1字节为单位, 内存划分的过于细碎, 很容易超出记录上限
*/
int memory_free_4k(struct MEMMNG *mng, unsigned int addr, unsigned int size) {
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memory_free(mng, addr, size);
    return i;
}
