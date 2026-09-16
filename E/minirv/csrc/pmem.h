#ifndef PMEM_H
#define PMEM_H

// 存储器容量 (字节). 改这里就够了, pmem.cpp 与加载检查都依赖它
#define PMEM_SIZE 0x100000 // 1MB, 先给够用的即可

// 从文本 hex 文件装入程序镜像 (每行一个 32 位字, 小端写入)
// 返回装入的 4 字节字数, 失败返回 -1
int pmem_load(const char *path);

#endif
