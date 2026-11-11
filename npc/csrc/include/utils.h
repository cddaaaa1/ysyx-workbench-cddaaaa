#ifndef NPC_UTILS_H
#define NPC_UTILS_H

#include <stdint.h>

enum { NPC_RUNNING, NPC_STOP, NPC_END, NPC_ABORT, NPC_QUIT};
struct NPCState {
  int state = NPC_STOP;
  bool retired = false;
  uint32_t retire_pc = 0;
  uint32_t halt_pc = 0;
  uint32_t halt_ret = 1;
};
extern NPCState npc_state;
#endif
