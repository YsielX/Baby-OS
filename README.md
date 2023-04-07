# Baby-OS-kernel

**It's a very simple OS kernel based on RISCV and executed by QEMU following OS-labs.**

Currently, kernel booting, clock interrupt processing, kernel thread scheduling, virtual memory management, user mode switching, page fault exceptions, and some system calls (fork, read, write) are implemented.

### Usage

`qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -bios default `

The files run by the system are in the `user` folder

### TODO

A VFS and finally a shell are to be implemented.