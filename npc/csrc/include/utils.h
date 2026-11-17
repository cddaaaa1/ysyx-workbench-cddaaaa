#ifndef NPC_UTILS_H
#define NPC_UTILS_H

#include <stdint.h>

#define ANSI_FG_RED    "\033[91m"
#define ANSI_FG_YELLOW "\033[93m"
#define ANSI_NONE      "\033[0m"
#define ANSI_FMT(str, fmt) fmt str ANSI_NONE

enum { NPC_RUNNING, NPC_STOP, NPC_END, NPC_ABORT, NPC_QUIT};
struct NPCState {
  int state = NPC_STOP;
  bool retired = false;
  uint32_t retire_pc = 0;
  uint32_t retire_inst = 0;
  uint32_t halt_pc = 0;
  uint32_t halt_ret = 1;
};
extern NPCState npc_state;
#endif
