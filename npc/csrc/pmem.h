#ifndef PMEM_H
#define PMEM_H

// 存储器基址与容量 (字节). AM 的 minirv-npc 运行时环境约定程序从 0x80000000 开始
#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000 // 128MB

// 从二进制镜像文件装入程序 (文件内容即从 PMEM_BASE 开始的内存镜像)
// 返回装入的字节数, 失败返回 -1
int pmem_load(const char *path);

// 设备读出值的 getter: DUT 读设备时存入 pmem.cpp 内部的 static 变量,
// 参考模型通过这几个函数读, 从而与 DUT 看到同一个值 (两者不能各自访问设备,
// 否则随机数/时间不同会导致 DiffTest 控制流分叉)
uint32_t pmem_uart_status(void);
uint32_t pmem_rtc_lo(void);
uint32_t pmem_rtc_hi(void);

#endif
