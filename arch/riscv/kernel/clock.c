#include "sbi.h"
// QEMU中时钟的频率是10MHz, 也就是1秒钟相当于10000000个时钟周期。
unsigned long TIMECLOCK = 10000000;
unsigned long get_cycles() {
	unsigned long tim;
	__asm__ volatile (
		"rdtime %[tim]\n"
		:[tim]"=r"(tim)
	);
	return tim;
}
void clock_set_next_event() {
// 下一次 时钟中断 的时间点
	unsigned long next = get_cycles() + TIMECLOCK;
// 使用 sbi_ecall 来完成对下一次时钟中断的设置
	sbi_ecall(0,0,next,0,0,0,0,0);
}

