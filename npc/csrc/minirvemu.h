#ifndef MINIRVEMU_H
#define MINIRVEMU_H

#include <stdint.h>

// 与仿真环境的 PMEM_BASE / PMEM_SIZE 保持一致, 否则 DiffTest 的访存结果会不一致
#define REF_MEM_BASE  0x80000000u
#define REF_MEM_SIZE  0x8000000 // 128MB
#define REF_MEM_WORDS (REF_MEM_SIZE / 4)

#define REF_REGISTER_COUNT 32

// 从二进制镜像文件装入程序 (文件内容即从 REF_MEM_BASE 开始的内存镜像)
// 返回装入的字节数, 失败时返回 -1
int ref_load_program(const char *path);

// 直接把指令镜像装入 M (不经文件), 返回装入的指令条数, 失败时返回 -1
int ref_load_image(const uint32_t *insts, int count);

// 将参考模型复位到 minirv-npc 的复位状态 (PC=0x80000000, GPR 全0)
void ref_reset(void);

// 参考模型执行一条指令。
// 返回 0: 正常执行; 返回 1: 执行了 ebreak, 即程序自行声明结束; 返回 -1: 非法指令
int ref_inst_cycle(void);

// 获得参考模型的 GPR 与 PC, 供 DiffTest 与 DUT 比较
uint32_t *ref_get_regs(void);
uint32_t ref_get_pc(void);

#endif
