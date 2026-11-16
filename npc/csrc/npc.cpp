#include "include/npc.h"
#ifdef ITRACE
#include "include/trace.h"
#endif
#ifdef DIFFTEST
#include "include/difftest.h"
#endif
#include <stdio.h>
#include <inttypes.h>

#ifndef MAX_CYCLE
#define MAX_CYCLE 200000000ULL
#endif

#define MAX_INST_TO_PRINT 10

NPCState npc_state;
static bool g_print_step = false;
static uint64_t total_cycles = 0;
static uint64_t total_instructions = 0;

void sim_retire(int pc, int inst) {
  npc_state.retire_pc = uint32_t(pc);
  npc_state.retire_inst = uint32_t(inst);
  npc_state.retired = true;
}

void ebreak(int code) {
  npc_state.halt_ret = uint32_t(code);
  npc_state.state = NPC_END;
}

static bool exec_once() {
  while (npc_state.state == NPC_RUNNING) {
    if (sim_finished() || total_cycles >= MAX_CYCLE) {
      fprintf(stderr, "NPC stopped before program completion (%s)\n",
                   sim_finished() ? "simulator finished" : "cycle limit");
      npc_state.state = NPC_ABORT;
      npc_state.halt_pc = npc_state.retire_pc;
      return false;
    }
    npc_state.retired = false;
    single_cycle();
    ++total_cycles;
    // Both DPI callbacks have completed after the full cycle.
    if (npc_state.state == NPC_END)
      npc_state.halt_pc = npc_state.retire_pc;
    if (npc_state.retired) return true;
  }
  return false;
}

static void execute(uint64_t n) {
  while (n > 0 && npc_state.state == NPC_RUNNING) {
    if (exec_once()) {
      ++total_instructions;
      --n;
#ifdef ITRACE
      itrace(npc_state.retire_pc, npc_state.retire_inst, g_print_step);
#endif
#ifdef DIFFTEST
      uint32_t npc;
      sim_read_pc(npc);
      difftest_step(npc_state.retire_pc, npc);
#endif
    }
#ifdef ITRACE
    if (npc_state.state == NPC_ABORT) iringbuf_dump();
#endif
  }
}

void npc_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  if (npc_state.state == NPC_END || npc_state.state == NPC_ABORT) {
    puts("Program execution has ended. Restart NPC to run again.");
    return;
  }
  npc_state.state = NPC_RUNNING;
  execute(n);
  if (npc_state.state == NPC_RUNNING) npc_state.state = NPC_STOP;
  else if (npc_state.state == NPC_END)
    printf("NPC: %s at pc = 0x%08x (code = %u)\n",
                npc_state.halt_ret == 0 ? "GOOD TRAP" : "BAD TRAP",
                npc_state.halt_pc, npc_state.halt_ret);
  else if (npc_state.state == NPC_ABORT)
    fprintf(stderr, "NPC: ABORT (last retired pc = 0x%08x)\n",
                 npc_state.retire_pc);
}

void npc_statistics() {
  const double ipc = total_cycles ? double(total_instructions) / total_cycles : 0;
  printf("Simulation: %" PRIu64 " cycles, %" PRIu64
              " instructions, IPC = %.4f, last pc = 0x%08x\n",
              total_cycles, total_instructions, ipc, npc_state.retire_pc);
}
