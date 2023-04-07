#include "sbi.h"

struct sbiret sbi_ecall(int ext, int fid, uint64_t arg0,
			            uint64_t arg1, uint64_t arg2,
			            uint64_t arg3, uint64_t arg4,
			            uint64_t arg5) 
{
	struct sbiret temp;
	long error;
	long value;
	__asm__ volatile (
	   "li a7,0\n"
	   "li a6,0\n"
	   "lw a7,%[ext]\n"
	   "lw a6,%[fid]\n"
	   "ld a5,%[arg5]\n"
	   "ld a4,%[arg4]\n"
	   "ld a3,%[arg3]\n"
	   "ld a2,%[arg2]\n"
	   "ld a1,%[arg1]\n"
	   "ld a0,%[arg0]\n"
	   "ecall\n"
	   "mv %[error],a0\n"
	   "mv %[value],a1\n"
	   :[error]"=r"(error),[value]"=r"(value)
	   :[arg5]"m"(arg5),[arg4]"m"(arg4),[arg3]"m"(arg3),[arg2]"m"(arg2),[arg1]"m"(arg1),[arg0]"m"(arg0),[fid]"m"(fid),[ext]"m"(ext)
	   :"memory"
	   );
	temp.error=error;
	temp.value=value;
	return temp;
}
