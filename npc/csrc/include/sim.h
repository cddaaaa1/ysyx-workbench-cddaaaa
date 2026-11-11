#ifndef NPC_SIM_H
#define NPC_SIM_H

#include <stdint.h>

void sim_init(int argc, char *argv[]);
void sim_reset(int cycles);
void single_cycle();
bool sim_finished();
void sim_cleanup();
int sim_gpr_count();
bool sim_read_gpr(int index, uint32_t &value);
bool sim_read_pc(uint32_t &value);

#endif
