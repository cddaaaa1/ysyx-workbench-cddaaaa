#ifndef NPC_REG_H
#define NPC_REG_H
#include <stdint.h>

struct CPUState {
  uint32_t gpr[32];
  uint32_t pc;
};

extern CPUState cpu_state;

void reg_display();

#endif
