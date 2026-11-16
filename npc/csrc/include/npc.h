#ifndef NPC_H
#define NPC_H

#include "utils.h"
#include "macro.h"
#include "pmem.h"

#include "sim.h"

#define MAX_INST_TO_PRINT 10

extern "C" void sim_retire(int pc, int inst);
extern "C" void ebreak(int code);
void npc_exec(uint64_t n);
void npc_statistics();
#endif
