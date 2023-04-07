#include "printk.h"
#include "sbi.h"
#include "defs.h"
#include "proc.h"
#include "vm.h"

extern void test();
extern unsigned long swapper_pg_dir[512];

void bye() {
    char b[20]="[S] 2023 Bye oslab!\n\0";
    create_mapping(swapper_pg_dir,0x10000000,0x10000000,0x100,0b0111);
    for(int i=0;b[i]!=0;i++)
        *(volatile unsigned char *) 0x10000000 = b[i];

}

int start_kernel() {
    bye();
    schedule();
    test(); // DO NOT DELETE !!!
	return 0;
}
