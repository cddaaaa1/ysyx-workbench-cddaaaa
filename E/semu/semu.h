#ifndef SEMU_H
#define SEMU_H

#include <stdint.h>

#define REF_MEMORY_SIZE    256
#define REF_REGISTER_COUNT 4

// 装入程序镜像, 返回装入的指令条数, 失败时返回 -1
int ref_load_program(const char *path);

// 将参考模型复位到与 sCPU 复位后一致的状态 (PC=0, GPR 全0)
void ref_reset(void);

// 参考模型执行一条指令, 成功返回 0, 遇到非法指令返回 -1
int ref_inst_cycle(void);

// 获得参考模型的 GPR 与 PC, 供 DiffTest 与 DUT 比较
uint8_t *ref_get_regs(void);
uint8_t ref_get_pc(void);

#endif
