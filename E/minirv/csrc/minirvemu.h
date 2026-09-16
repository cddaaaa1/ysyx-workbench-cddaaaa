#ifndef MINIRVEMU_H
#define MINIRVEMU_H

#include <stdint.h>

#define REF_MEMORY_SIZE    256
#define REF_REGISTER_COUNT 16

// 从文件装入程序镜像, 返回装入的指令条数, 失败时返回 -1
int ref_load_program(const char *path);

// 直接把指令镜像装入 M (不经文件), 返回装入的指令条数, 失败时返回 -1
int ref_load_image(const uint32_t *insts, int count);

// 将参考模型复位到 ISA 规定的复位状态 (PC=0, GPR 全0)
void ref_reset(void);

// 参考模型执行一条指令。
// 返回 0: 正常执行; 返回 1: 执行了 ebreak, 即程序自行声明结束; 返回 -1: 非法指令
int ref_inst_cycle(void);

// 获得参考模型的 GPR 与 PC, 供 DiffTest 与 DUT 比较
uint32_t *ref_get_regs(void);
uint32_t ref_get_pc(void);

#endif
