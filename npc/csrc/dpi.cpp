#include "include/npc.h"

extern "C" void sim_retire(int pc, int inst) {
  (void)inst;
  npc_state.retire_pc = uint32_t(pc);
  npc_state.retired = true;
}

extern "C" void ebreak(int code) {
  npc_state.halt_ret = uint32_t(code);
  npc_state.state = NPC_END;
}
