#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "include/npc.h"

#define PROGRAM_PATH "../ysyxSoC/ready-to-run/minirv/hello-minirv-ysyxsoc.bin"
#define CLK_RATIO 1

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = new VerilatedVcdC;
static TOP *top = nullptr;

SimState sim;

void nvboard_bind_all_pins(TOP* top);

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    IFDEF(TRACE_ON, tfp->dump(contextp->time()));
}

void single_cycle() {
#ifdef YSYXSOC
  // clock 周期 = CLK_RATIO 个 cpuClock 周期, 占空比 50%
  static int clk_cnt = 0;
  top->cpuClock = 0; eval_and_dump();
  if (++clk_cnt >= CLK_RATIO) { clk_cnt = 0; top->clock = !top->clock; }
  top->cpuClock = 1; eval_and_dump();
  if (++clk_cnt >= CLK_RATIO) { clk_cnt = 0; top->clock = !top->clock; }
#else
  top->clock = 0; eval_and_dump();
  top->clock = 1; eval_and_dump();
#endif
}

static void reset(int cycles)
{
    top->reset = 1;
    while (cycles-- > 0)
        single_cycle();
    top->reset = 0;
}


int main(int argc, char **argv)
{   
    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    top = new TOP{contextp};

    IFDEF(NVBOARD, nvboard_bind_all_pins(top);nvboard_init());
    IFDEF(TRACE_ON, contextp->traceEverOn(true); top->trace(tfp, 0); tfp->open("dump.vcd"));

    const char *img = (argc > 1) ? argv[1] : PROGRAM_PATH;
#ifdef YSYXSOC
    flash_load(img);
#else
    pmem_load(img);
#endif
    reset(100);

    unsigned long long cycle = 0;
    long long inst_count = 0;

    for (; !contextp->gotFinish() IFDEF(DEBUG_TIME, && cycle < (unsigned long long)MAX_CYCLE); cycle++) {
        IFDEF(NVBOARD,nvboard_update());
        sim.cycle = cycle;
        sim.retired = false;
        single_cycle();

        if (sim.retired)
            inst_count++;
    }

    double ipc = (double)inst_count / (double)(cycle + 1);
#ifdef NETLIST_SIM
    // 网表里没有 DPI-C sim_retire, 数不出退休指令数
    printf("Simulation stopped after %llu cycles (netlist)\n", cycle + 1);
#else
    printf("Simulation stopped after %llu cycles, %lld instructions executed, IPC = %.4f (last pc = 0x%08x)\n",
           cycle + 1, inst_count, ipc, static_cast<unsigned>(sim.retire_pc));
#endif
    top->final();
    tfp->close();
    delete tfp;
    delete top;
    delete contextp;
    return 0;
}
