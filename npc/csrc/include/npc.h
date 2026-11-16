#ifndef NPC_H
#define NPC_H

#include "utils.h"
#include "macro.h"
#include "pmem.h"

#include "sim.h"

extern "C" void sim_retire(int pc, int inst);
extern "C" void ebreak(int code);
void npc_exec(uint64_t n);
void npc_statistics();
#endif
