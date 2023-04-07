#include "printk.h"
#include "proc.h"
#include "mm.h"
#include "vm.h"
#include "defs.h"
#include "syscall.h"
#include "string.h"
extern struct task_struct *current;
extern char uapp_start[], uapp_end[];
extern void clock_set_next_event();

void do_page_fault(struct pt_regs *regs)
{
	uint64_t wrong_addr = PGROUNDDOWN(regs->stval); // wrong_addr为出错地址所在虚拟页面的开头
	struct vm_area_struct *vma = find_vma(current, regs->stval);
	if (!vma)
		return;
	uint64_t newpage = alloc_page();
	create_mapping((uint64_t *)((uint64_t)current->pgd + PA2VA_OFFSET), wrong_addr, newpage - PA2VA_OFFSET, PGSIZE, vma->vm_flags | 0b10001);
	memset((char *)newpage, 0, PGSIZE);
	if ((vma->vm_flags & 1))
	{
		uint64_t copysize = vma->vm_content_size_in_file - (wrong_addr - vma->vm_start);
		if (copysize > 0)
		{
			if (copysize > PGSIZE)
			{
				copysize = PGSIZE;
			}
		}
		else
		{
			copysize = 0;
		}
		memcpy((char *)newpage, vma->file_offset_on_disk+vma->vm_content_offset_in_file+wrong_addr-vma->vm_start, copysize);
	}
}

void trap_handler(struct pt_regs *regs)
{
	unsigned long scause = regs->scause;
	if (scause == 0x8000000000000005)
	{ // check the time interrupt
		// printk("time interrupt\n");
		clock_set_next_event();
		do_timer();
	}
	else if (scause == 8)
	{
		syscall(regs);
		// printk("ecall from user mode!\n");
		regs->sepc += 4;
	}
	else if (scause == 12 || scause == 13 || scause == 15)
	{
		do_page_fault(regs);
	}
	else
	{
		printk("[S] Unhandled trap, ");
		printk("scause: %lx, ", scause);
		printk("stval: %lx, ", regs->stval);
		printk("sepc: %lx\n", regs->sepc);
		while (1)
			;
	}
}
