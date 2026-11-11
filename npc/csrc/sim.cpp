#include <cstdlib>
#include <cstring>
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "include/npc.h"
#include "include/sim.h"

#ifdef YSYXSOC
#include "VSimTop.h"
#include "VSimTop___024root.h"
using TOP = VSimTop;
#else
#include "Vnpc_top.h"
#include "Vnpc_top___024root.h"
using TOP = Vnpc_top;
#endif
#ifdef NVBOARD
#include <nvboard.h>
void nvboard_bind_all_pins(TOP *top);
#endif

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static TOP *top = nullptr;

static const auto &gpr_file() {
#ifdef YSYXSOC
  return top->rootp->SimTop__DOT__asic__DOT__soc__DOT__cpu__DOT__core0__DOT__u_gpr__DOT__rf;
#else
  return top->rootp->npc_top__DOT__npc__DOT__u_gpr__DOT__rf;
#endif
}

int sim_gpr_count() {
  if (!top) return 0;
  const auto &regs = gpr_file();
  return sizeof(regs) / sizeof(regs[0]);
}

bool sim_read_gpr(int index, uint32_t &value) {
  if (index < 0 || index >= sim_gpr_count()) return false;
  value = (index == 0) ? 0 : gpr_file()[index];
  return true;
}

bool sim_read_pc(uint32_t &value) {
  if (!top) return false;
#ifdef YSYXSOC
  value = top->rootp->SimTop__DOT__asic__DOT__soc__DOT__cpu__DOT__core0__DOT__pc;
#else
  value = top->rootp->npc_top__DOT__npc__DOT__pc;
#endif
  return true;
}

static void eval_and_dump() {
  top->eval();
  contextp->timeInc(1);
  IFDEF(TRACE_ON, tfp->dump(contextp->time()));
}

bool sim_finished() { return contextp->gotFinish(); }

void single_cycle() {
#ifdef YSYXSOC
  top->cpuClock = 0; eval_and_dump();
  top->clock = !top->clock;
  top->cpuClock = 1; eval_and_dump();
  top->clock = !top->clock;
#else
  top->clock = 0; eval_and_dump();
  top->clock = 1; eval_and_dump();
#endif
  IFDEF(NVBOARD, nvboard_update());
}

void sim_reset(int cycles) {
  top->reset = 1;
  while (cycles-- > 0) single_cycle();
  top->reset = 0;
  npc_state = NPCState{};
}

void sim_init(int argc, char *argv[]) {
  contextp = new VerilatedContext;
  contextp->commandArgs(argc, argv);
  top = new TOP{contextp};
  IFDEF(NVBOARD, nvboard_bind_all_pins(top); nvboard_init());
#ifdef TRACE_ON
  tfp = new VerilatedVcdC;
  contextp->traceEverOn(true);
  top->trace(tfp, 0);
  tfp->open("dump.vcd");
#endif
}

void sim_cleanup() {
  if (!top) return;
  top->final();
  if (tfp) { tfp->close(); delete tfp; tfp = nullptr; }
  IFDEF(NVBOARD, nvboard_quit());
  delete top;
  top = nullptr;
  delete contextp;
  contextp = nullptr;
}
