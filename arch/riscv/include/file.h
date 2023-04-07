#include "stdint.h"

struct file {
    enum {
        FD_NONE, FD_INIT, FD_OPENED, FD_CLOSED,
    } status;                         //访问文件的执行状态
    bool readable;                    //文件是否可读
    bool writable;                    //文件是否可写
    int fd;                           //文件在filemap中的索引值
    uint64_t pos;                     //访问文件的当前位置
    struct inode *node;               //该文件对应的内存inode指针
    int open_count;                   //打开此文件的次数
};