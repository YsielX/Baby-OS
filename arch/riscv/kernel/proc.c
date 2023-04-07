#include "proc.h"
#include "defs.h"
#include "mm.h"
#include "rand.h"
#include "printk.h"
#include "elf.h"
#include "string.h"
#include "vm.h"
extern void __dummy();
extern void __switch_to(struct task_struct *prev, struct task_struct *next);
extern char uapp_start[], uapp_end[];
extern unsigned long swapper_pg_dir[512];
struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 `task_struct`
struct task_struct *task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

static uint64_t load_program(struct task_struct *task, pagetable_t pgtbl)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)uapp_start;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr *phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++)
    {
        phdr = (Elf64_Phdr *)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD)
        {
            // do mapping
            // code...
            pagetable_t user_space = (pagetable_t)alloc_pages(phdr->p_memsz / PGSIZE + 1);
            memcpy((char *)((uint64_t)user_space + phdr->p_offset % PGSIZE), (char *)(uapp_start + phdr->p_offset), phdr->p_filesz);
            memset((char *)((uint64_t)user_space + phdr->p_offset % PGSIZE + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);

            uint64_t va = PGROUNDDOWN(phdr->p_vaddr);
            uint64_t pa = (uint64_t)user_space - PA2VA_OFFSET; // 创建用户态中uapp的va到pa的映射
            create_mapping(pgtbl, va, pa, phdr->p_memsz, 31);
        }
    }

    // allocate user stack and do mapping
    // code...
    pagetable_t user_stack = (pagetable_t)alloc_page();                                        //??????????????????????
    create_mapping(pgtbl, USER_END - PGSIZE, (uint64_t)user_stack - PA2VA_OFFSET, PGSIZE, 31); // 映射user_stack
    task->pgd = (pagetable_t)((uint64_t)pgtbl - PA2VA_OFFSET);

    // following code has been written for you
    // pc for the user program
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    task->thread.sstatus = csr_read(sstatus);
    task->thread.sstatus &= ~(1 << 8); // SPP
    task->thread.sstatus |= 1 << 5;    // SPIE
    task->thread.sstatus |= 1 << 18;   // SUM
    // user stack for user program
    task->thread.sscratch = USER_END - 8;
}

static void vma_init(struct task_struct *task)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)uapp_start;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr *phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++)
    {
        phdr = (Elf64_Phdr *)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD)
        {
            // do mapping
            // code...
            int R = (phdr->p_flags >> 2) & 1;
            int W = (phdr->p_flags >> 1) & 1;
            int X = (phdr->p_flags) & 1;
            uint64_t flags = (R << 1) | (W << 2) | (X << 3) | VM_ANONYM;
            do_mmap(task, phdr->p_vaddr, phdr->p_memsz, flags, uapp_start, phdr->p_offset, phdr->p_filesz);
        }
    }

    do_mmap(task, USER_END - PGSIZE, PGSIZE, VM_R_MASK | VM_W_MASK, uapp_start, 0, 0);

    // pc for the user program
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    task->thread.sstatus = csr_read(sstatus);
    task->thread.sstatus &= ~(1 << 8); // SPP
    task->thread.sstatus |= 1 << 5;    // SPIE
    task->thread.sstatus |= 1 << 18;   // SUM
    // user stack for user program
    task->thread.sscratch = USER_END - 8;
}

uint64_t find_new_task(struct task_struct *a)
{
    for (int i = 1; i < NR_TASKS; i++)
    {
        if (!task[i])
        {
            task[i] = a;
            return i;
        }
    }
    return 0;
}

void task_init()
{
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    struct task_struct *tempr = (struct task_struct *)kalloc();
    idle = tempr;
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;

    tempr = (struct task_struct *)kalloc();
    task[1] = tempr;
    task[1]->state = TASK_RUNNING;
    task[1]->counter = 0;
    task[1]->priority = rand() % 11; // priority upperbound is 10
    // printk("task[%d] priority is %d\n", 1, task[1]->priority);
    task[1]->pid = 1;
    task[1]->thread.ra = (uint64_t)__dummy;
    task[1]->thread.sp = (uint64_t)tempr + PGSIZE - 8;
    task[1]->vma_cnt = 0;

    pagetable_t pgtbl = (pagetable_t)alloc_page();
    memcpy((char *)pgtbl, (char *)swapper_pg_dir, PGSIZE); // 拷贝内核态顶级页表给用户态
    task[1]->pgd = (pagetable_t)((uint64_t)pgtbl - PA2VA_OFFSET);

    vma_init(task[1]);

    // load_program(task[i], pgtbl);

    printk("...proc_init done!\n\n");
}

void dummy()
{
    uint64_t MOD = 1000000007;
    uint64_t auto_inc_local_var = 0;
    int last_counter = -1;
    while (1)
    {
        if (last_counter == -1 || current->counter != last_counter)
        {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            if (current->pid == 0)
                printk("[S-Mode]");
            else
                printk("[U-Mode]!!!");
            printk("[PID = %d] is running. thread space begin at 0x%llx\n", current->pid, current);
        }
    }
}

void switch_to(struct task_struct *next)
{
    if (next != current)
    {
        struct task_struct *temp = current;
        current = next;
        __switch_to(temp, next);
    }
}
void do_timer(void)
{
    if (current == idle)
    {
        schedule();
    }
    else
    {
        current->counter--;
        if (current->counter > 0)
            return;
        else
            schedule();
    }
}

void schedule()
{
#ifdef SJF
    struct task_struct *min;
    int flag = 0;
    min = task[1];                     // min is the task we want to find
    for (int i = 1; i < NR_TASKS; i++) // 判断全0
    {
        if (task[i])
        {
            if (task[i]->counter != 0 && task[i]->state == TASK_RUNNING)
                flag = 1; // if there is a counter that is not 0
        }
    }
    if (flag == 0)
    { // 用rand重置全0的counter
        for (int i = 1; i < NR_TASKS; i++)
        {
            if (task[i])
            {
                task[i]->counter = rand();
                printk("[S] reset pid %d's counter to %d\n", task[i]->pid, task[i]->counter);
            }
        }
    }
    int temp1 = 1;
    for (int i = 2; i < NR_TASKS; i++) // find the minimal counter
    {
        if (task[i])
        {
            if ((min->counter == 0 || min->state != TASK_RUNNING))
            {
                min = task[i];
                temp1 = i;
            }
            if ((min->counter > task[i]->counter && task[i]->counter != 0 && task[i]->state == TASK_RUNNING))
            {
                min = task[i];
                temp1 = i;
            }
        }
    }
    printk("[S] change to thread %d, counter is %d!\n", temp1, min->counter);
    switch_to(min);
#endif
#ifdef PRIORITY
    struct task_struct *max;
    int flag = 0;
    max = task[1];
    int temp1 = 1;
    for (int i = 1; i < NR_TASKS; i++)
    {
        if (task[i])
        {
            if (task[i]->counter != 0 && task[i]->state == TASK_RUNNING)

                flag = 1; // if there is a counter that is not 0
        }
    }
    if (flag == 0)
    { // 如果running的全为0
        printk("counter reset!!!!!!!!!!!!!!\n");
        for (int i = 1; i < NR_TASKS; i++)
        {
            if (task[i])
            {
                task[i]->counter = (task[i]->counter >> 1) + task[i]->priority; // 用priority更新counter
                printk("task[%d]->counter is %d\n", i, task[i]->counter);
            }
        }
    }
    for (int i = 2; i < NR_TASKS; i++)
    {
        if (task[i])
        {
            if (max->state != TASK_RUNNING)
            {
                max = task[i];
                temp1 = i;
            }

            if (max->counter < task[i]->counter && task[i]->state == TASK_RUNNING)
            {
                max = task[i];
                temp1 = i;
            }
        }
    }
    printk("change to thread %d\n", temp1);
    printk("counter is %d!\n", max->counter);
    printk("priority is %d!!\n", max->priority);
    switch_to(max);

#endif
}

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
            uint64_t file_offset_on_disk, uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file)
{
    uint64_t cnt = task->vma_cnt;
    task->vmas[cnt].vm_start = addr;
    task->vmas[cnt].vm_end = addr + length;
    task->vmas[cnt].vm_flags = flags;
    task->vmas[cnt].file_offset_on_disk = file_offset_on_disk;
    task->vmas[cnt].vm_content_offset_in_file = vm_content_offset_in_file;
    task->vmas[cnt].vm_content_size_in_file = vm_content_size_in_file;
    task->vma_cnt++;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr)
{
    for (int i = 0; i < task->vma_cnt; i++)
    {
        if (task->vmas[i].vm_start <= addr && task->vmas[i].vm_end > addr)
        {
            return &task->vmas[i];
        }
    }
    return NULL;
}