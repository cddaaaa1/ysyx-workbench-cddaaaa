#ifndef PMEM_H
#define PMEM_H

// 存储器基址与容量 (字节). AM 的 minirv-npc 运行时环境约定程序从 0x80000000 开始
#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000 // 128MB

// 从二进制镜像文件装入程序 (文件内容即从 PMEM_BASE 开始的内存镜像)
// 返回装入的字节数, 失败返回 -1
int pmem_load(const char *path);

#endif
