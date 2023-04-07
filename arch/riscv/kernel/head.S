#include "defs.h"
.extern start_kernel
//.extern mm_init
.extern task_init
.extern setup_vm_final
.extern setup_vm
.extern early_pgtbl

    .section .text.init #put _start into .text.init.section
    .globl _start
_start:
    # YOUR CODE HERE
    
    la sp,boot_stack_top  //因为后面涉及函数跳转，所以栈顶肯定是第一个设置的
    call setup_vm  //两个映射
    call relocate  //这个函数设置好satp寄存器，然后把ra寄存器增加到虚拟地址，ret到虚拟地址通过satp寄存器找到对应页表，回到对应的物理地址
    jal mm_init
    call setup_vm_final //设置三级页表映射
    jal task_init     
    #--------------------set stvec = _traps
    
    la a0,_traps
    csrw stvec,a0
    
    #--------------------set sie[STIE]=1
    
    csrr t0,sie
    ori t0,t0,0x20
    csrw sie,t0
    
    #--------------------set first time interrupt
    
    rdtime t0
    xor t1,t1,t1
    lui t1,0x989
    addi t1,t1,0x680
    add t0,t0,t1
    xor a7,a7,a7
    xor a6,a6,a6
    xor a5,a5,a5
    xor a4,a4,a4
    xor a3,a3,a3
    xor a2,a2,a2
    xor a1,a1,a1
    mv a0,t0
    ecall
    
    #--------------------set sstatus[SIE]=1
    # csrr t0,sstatus
    # ori t0,t0,0x2
    # csrw sstatus,t0
    #--------------------
    # ------------------    
    j start_kernel
    # ------------------
relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0,PA2VA_OFFSET
    add ra,ra,t0
    add sp,sp,t0
    
    # set satp with early_pgtbl
    la t0,early_pgtbl
    srli t0,t0,12
    li t1,1
    slli t1,t1,63
    or t0,t0,t1
    csrw satp,t0//把satp设置成earlytable所在的物理页表
    
    # flush tlb
    sfence.vma zero, zero

    ret
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size
    
    .globl boot_stack_top
boot_stack_top:
