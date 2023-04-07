#include "defs.h"
#include "stdint.h"
#include "string.h"
#include "printk.h"
#include "mm.h"
#include "vm.h"

unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000))); //顶级页表

void setup_vm(void)
{
    unsigned long PA = (((PHY_START >> 12) << 10) | 0b1111); //构造页表项结构//PPN（44位）|flags（10位），因为是4KB=2^12所以右移12表示页号
    early_pgtbl[VM_START >> 30 & 0x1ff] = PA;                //获取L2（在第31位），得到对应表项的PPN的值改成Pagetable_Entry
    early_pgtbl[PHY_START >> 30 & 0x1ff] = PA;               //获取L2（在第31位），得到对应表项的PPN的值改成Pagetable_Entry
    printk("setup_vm!\n");
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
extern char _stext[], _etext[], _srodata[], _erodata[];
extern char _sdata[], _ebss[];

void setup_vm_final(void)
{
    memset(swapper_pg_dir, 0x0, PGSIZE);
    // No OpenSBI mapping required
    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_stext, (uint64_t)_stext - PA2VA_OFFSET, (uint64_t)_etext - (uint64_t)_stext, 0b1011);
    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_srodata, (uint64_t)_srodata - PA2VA_OFFSET, (uint64_t)_erodata - (uint64_t)_srodata, 0b0011);
    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_sdata, (uint64_t)_sdata - PA2VA_OFFSET, PHY_SIZE - ((uint64_t)_sdata - (uint64_t)_stext), 0b0111);
    // set satp with swapper_pg_dir
    // csr_write(satp,swapper_pg_dir);
    uint64_t PPN = ((uint64_t)swapper_pg_dir - PA2VA_OFFSET) >> 12, ASID = 0, MODE = 8;
    uint64_t temp = PPN | (ASID << 44) | (MODE << 60);
    csr_write(satp, temp);
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    return;
}

int has_mapping(uint64_t* pgtbl,uint64_t va)
{
    int l2 = (va >> 30) & 0x1ff;
    int l1 = (va >> 21) & 0x1ff;
    int l0 = (va >> 12) & 0x1ff;

    uint64_t page2=pgtbl[l2];
    if(!(page2&1))
        return 0;
    
    uint64_t *pgtbl1 = (uint64_t *)(((pgtbl[l2] >> 10) << 12) + PA2VA_OFFSET); //一级页表的最开始的地址
    uint64_t page1 = pgtbl1[l1];
    if(!(page1&1))
        return 0;
    
    uint64_t *pgtbl0 = (uint64_t *)(((pgtbl1[l1] >> 10) << 12) + PA2VA_OFFSET);
    uint64_t page0=pgtbl0[l0];
    if(!(page0&1))
        return 0;
    
    return 1;
}

/* 创建多级页表映射关系 */
void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm)
{
    for (uint64_t va_i = PGROUNDDOWN(va); va_i <= PGROUNDUP(sz + va - 1); va_i += PGSIZE, pa += PGSIZE) //遍历每一页开始的地址，把va变成本页最开始的地址
    {
        int l2 = (va_i >> 30) & 0x1ff;
        int l1 = (va_i >> 21) & 0x1ff;
        int l0 = (va_i >> 12) & 0x1ff;
        // int offset = va_i & 0xfff;//error
        uint64_t page2 = pgtbl[l2];
        if (!(page2 & 1)) //如果没找到一级的page，说明要分配一个新的page作为一级页表
        {
            uint64_t newpgtbl1 = kalloc();
            pgtbl[l2] = (((newpgtbl1 - PA2VA_OFFSET) >> 12) << 10) | 1; //填二级page对应的表项指向刚申请的一级页表
        }
        uint64_t *pgtbl1 = (uint64_t *)(((pgtbl[l2] >> 10) << 12) + PA2VA_OFFSET); //一级页表的最开始的地址
        uint64_t page1 = pgtbl1[l1];
        if (!(page1 & 1)) //如果没找到零级的page，说明要分配一个新的page作为零级页表
        {
            uint64_t newpgtbl0 = kalloc();
            pgtbl1[l1] = (((newpgtbl0 - PA2VA_OFFSET) >> 12) << 10) | 1; //填1级page对应的表项指向刚申请的0级页表
        }
        uint64_t *pgtbl0 = (uint64_t *)(((pgtbl1[l1] >> 10) << 12) + PA2VA_OFFSET);
        pgtbl0[l0] = ((pa >> 12) << 10) | perm;
    }
}
