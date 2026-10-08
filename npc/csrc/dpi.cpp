#include <stdint.h>

#include "include/npc.h"

// 供仿真环境判断"本拍提交了一条指令"
extern "C" void sim_retire(int pc, int inst)
{
	sim.retire_pc = (uint32_t)pc;
	sim.retired   = true;
}
