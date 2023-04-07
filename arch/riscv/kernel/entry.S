.extern trap_handler
.extern dummy

.section .text.entry
.align 2
.globl _traps
.globl __dummy
.globl __switch_to
_traps:
	#YOUR CODE HERE
	#-------------切换页表,跳转到kernelmode,这里舍弃了t0
	csrr t0,sscratch
	beq t0, zero, _noswitch#跳过下两行
	csrw sscratch,sp
	mv sp,t0
_noswitch:
	#-------------save 32 registers and sepc,sstatus,stval,sscratch,scause to stack
	addi sp,sp,-296
	csrr t0,sepc
	sd t0,256(sp)
	csrr t0,sstatus
	sd t0,264(sp)
	csrr t0,stval
	sd t0,272(sp)
	csrr t0,sscratch
	sd t0,280(sp)
	csrr t0,scause
	sd t0,288(sp)
	sd t0,0(sp)
	sd t1,8(sp)
	sd t2,16(sp)
	sd t3,24(sp)
	sd t4,32(sp)
	sd t5,40(sp)
	sd t6,48(sp)
	sd zero,56(sp)
	sd a0,64(sp)
	sd a1,72(sp)
	sd a2,80(sp)
	sd a3,88(sp)
	sd a4,96(sp)
	sd a5,104(sp)
	sd a6,112(sp)
	sd a7,120(sp)
	sd s0,128(sp)
	sd s1,136(sp)
	sd s2,144(sp)
	sd s3,152(sp)
	sd s4,160(sp)
	sd s5,168(sp)
	sd s6,176(sp)
	sd s7,184(sp)
	sd s8,192(sp)
	sd s9,200(sp)
	sd s10,208(sp)
	sd s11,216(sp)
	sd ra,224(sp)
	sd tp,232(sp)
	sd gp,240(sp)
	mv t0,sp
	addi t0,t0,296
	sd t0,248(sp)

	#-------------call trap_handler
	mv a0,sp
	jal trap_handler

	.global __ret_from_fork
__ret_from_fork:

	#-------------restore sepc and 32 registers (x2(sp) should be restore last) from stack
	ld t0,256(sp)
	csrw sepc,t0
	ld t0,264(sp)
	csrw sstatus,t0
	ld t0,272(sp)
	csrw stval,t0
	ld t0,280(sp)
	csrw sscratch,t0
	ld t0,288(sp)
	csrw scause,t0
	ld t0,0(sp)
	ld t1,8(sp)
	ld t2,16(sp)
	ld t3,24(sp)
	ld t4,32(sp)
	ld t5,40(sp)
	ld t6,48(sp)
	#ld zero,56(sp)
	ld a0,64(sp)
	ld a1,72(sp)
	ld a2,80(sp)
	ld a3,88(sp)
	ld a4,96(sp)
	ld a5,104(sp)
	ld a6,112(sp)
	ld a7,120(sp)
	ld s0,128(sp)
	ld s1,136(sp)
	ld s2,144(sp)
	ld s3,152(sp)
	ld s4,160(sp)
	ld s5,168(sp)
	ld s6,176(sp)
	ld s7,184(sp)
	ld s8,192(sp)
	ld s9,200(sp)
	ld s10,208(sp)
	ld s11,216(sp)
	ld ra,224(sp)
	ld tp,232(sp)
	ld gp,240(sp)
	ld sp,248(sp)

	#-------------切换页表，跳回user
	csrr t0,sscratch
	beq t0, zero, _noswitch2#跳过下两行
	csrw sscratch,sp
	mv sp,t0
_noswitch2:
	#-------------return from trap
	sret
	#-------------

__switch_to:
	sd ra,32(a0)
	sd sp,40(a0)
	sd s0,48(a0)
	sd s1,56(a0)
	sd s2,64(a0)
	sd s3,72(a0)
	sd s4,80(a0)
	sd s5,88(a0)
	sd s6,96(a0)
	sd s7,104(a0)
	sd s8,112(a0)
	sd s9,120(a0)
	sd s10,128(a0)
	sd s11,136(a0)
	csrr s0,sepc
	csrr s1,sstatus
	csrr s2,sscratch
	sd s0,144(a0)
	sd s1,152(a0)
	sd s2,160(a0)
	
	ld ra,32(a1)
	ld sp,40(a1)
	ld s0,144(a1)
	ld s1,152(a1)
	ld s2,160(a1)
	csrw sepc,s0
	csrw sstatus,s1
	csrw sscratch,s2

	ld s0,168(a1)#用新的页表替换原来的
	srli s0,s0,12
	li s1,1
	slli s1,s1,63
	or s0,s0,s1
	csrw satp,s0

	ld s0,48(a1)
	ld s1,56(a1)
	ld s2,64(a1)
	ld s3,72(a1)
	ld s4,80(a1)
	ld s5,88(a1)
	ld s6,96(a1)
	ld s7,104(a1)
	ld s8,112(a1)
	ld s9,120(a1)
	ld s10,128(a1)
	ld s11,136(a1)


	sfence.vma zero, zero
	
	ret
	
__dummy:

	csrr t0,sscratch
	csrw sscratch,sp
	mv sp,t0
	# la t0,dummy
	# csrw sepc,t0 #设置spec为dummy地址返回
	sret
