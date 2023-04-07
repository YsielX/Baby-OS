#include "syscall.h"
#include "printk.h"
#include "defs.h"
#include "vm.h"
#include "mm.h"
#include "rand.h"
#include "string.h"

extern struct task_struct *current;
extern void __ret_from_fork();
extern unsigned long swapper_pg_dir[512];

void syscall(struct pt_regs *regs)
{
    if (regs->a[7] == SYS_WRITE)
    {
        sys_write(regs->a[0], (const char *)regs->a[1], regs->a[2]);
    }
    else if (regs->a[7] == SYS_GETPID)
    {
        regs->a[0] = sys_getpid();
    }
    else if (regs->a[7] == SYS_CLONE)
    {
        sys_clone(regs);
    }
}

void sys_write(unsigned int fd, const char *buf, uint64_t count)
{
    if (fd == 1)
    {
        for (int i = 0; i < count; i++)
        {
            printk("%c", buf[i]);
        }
    }
}

uint64_t sys_getpid()
{
    return current->pid;
}

uint64_t sys_clone(struct pt_regs *regs)
{
    struct task_struct *tempr = (struct task_struct *)alloc_page();
    memcpy(tempr, current, PGSIZE);
    tempr->priority = rand() % 11; // priority upperbound is 10
    // printk("task[%d] priority is %d\n", i, tempr->priority);

    tempr->pid = find_new_task(tempr);
    tempr->thread.ra = (uint64_t)__ret_from_fork;
    tempr->thread.sp = (uint64_t)tempr + ((uint64_t)regs - (uint64_t)current);

    struct pt_regs *child_regs = (uint64_t)tempr + ((uint64_t)regs - (uint64_t)current);
    child_regs->a[0] = 0;
    child_regs->sp = tempr->thread.sp + 296;
    child_regs->sepc = regs->sepc + 4;

    uint64_t child_userstack = alloc_page(); // 默认栈的大小不超过一页
    memcpy(child_userstack, PGROUNDDOWN(current->thread.sscratch), PGSIZE);
    tempr->thread.sscratch = child_userstack+PGSIZE-8;

    pagetable_t pgtbl = (pagetable_t)alloc_page();
    memcpy(pgtbl, swapper_pg_dir, PGSIZE);
    for (int i = 0; i < current->vma_cnt; i++)
    {
        struct vm_area_struct *vma = &current->vmas[i];
        for (uint64_t j = vma->vm_start; j < vma->vm_end; j += PGSIZE)
        {
            if (!has_mapping((uint64_t *)((uint64_t)current->pgd + PA2VA_OFFSET), j))
                continue;
            uint64_t child_page = alloc_page();
            create_mapping(pgtbl, PGROUNDDOWN(j), child_page - PA2VA_OFFSET, PGSIZE, 0b10001 | vma->vm_flags);
            memcpy(child_page, PGROUNDDOWN(j), PGSIZE);
        }
    }
    tempr->pgd = (pagetable_t)((uint64_t)pgtbl - PA2VA_OFFSET);

    return tempr->pid;
}
