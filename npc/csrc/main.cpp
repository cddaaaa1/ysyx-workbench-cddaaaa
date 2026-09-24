#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/VSimTop.h"
#include "minirvemu.h"
#include "include/npc.h"

#define PROGRAM_PATH "../ysyxSoC/ready-to-run/minirv/hello-minirv-ysyxsoc.bin"
#define MAX_CYCLES 1e8

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static VSimTop *top = nullptr;

SimState sim;

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    if (tfp) tfp->dump(contextp->time());
}

void single_cycle() {
  top->clock = 0; top->cpuClock = 0; eval_and_dump();
  top->clock = 1; top->cpuClock = 1; eval_and_dump();
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
    const char *img = (argc > 1) ? argv[1] : PROGRAM_PATH;

    flash_load(img);

    ref_reset();
    
    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    top = new VSimTop{contextp};

    const char *vcd = getenv("NPC_TRACE");
    if (vcd != nullptr && *vcd != '\0') {
        const char *depth = getenv("NPC_TRACE_DEPTH");
        contextp->traceEverOn(true);
        tfp = new VerilatedVcdC;
        top->trace(tfp, depth ? atoi(depth) : 1); 
        tfp->open(vcd);
    }
    reset(100); 

    unsigned long long cycle = 0;
    long long inst_count = 0;

    for (; !contextp->gotFinish(); cycle++) {
        sim.cycle = cycle;
        sim.retired = false;
        single_cycle();

        if (sim.retired)
            inst_count++;
    }

    printf("Simulation stopped after %llu cycles, %lld instructions executed (last pc = 0x%08x)\n",
           cycle + 1, inst_count, static_cast<unsigned>(sim.retire_pc));

    top->final();
    if (tfp) {
        tfp->close();
        delete tfp;
    }
    delete top;
    delete contextp;
    return 0;
}
