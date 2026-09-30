#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/VSimTop.h"
#include "minirvemu.h"
#include "include/npc.h"
#include <nvboard.h>  

#define PROGRAM_PATH "../ysyxSoC/ready-to-run/minirv/hello-minirv-ysyxsoc.bin"
#define MAX_CYCLES 1e8
#define CLK_RATIO 3

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static VSimTop *top = nullptr;

static int clk_cnt = 0;
static long long trace_cycles = 100000;  // 只 dump 前 N 个周期, 防止 VCD 过大

SimState sim;

// 由 build/auto_bind.cpp (auto_pin_bind.py 按 constr/top.nxdc 生成) 提供
void nvboard_bind_all_pins(VSimTop* top);

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    if (tfp) tfp->dump(contextp->time());
}

// clock 周期 = CLK_RATIO 个 cpuClock 周期, 占空比 50%
// 以 cpuClock 半拍为单位计数, 每 CLK_RATIO 个半拍翻转一次 clock
void single_cycle() {
  top->cpuClock = 0; eval_and_dump();
  if (++clk_cnt >= CLK_RATIO) { clk_cnt = 0; top->clock = !top->clock; }
  top->cpuClock = 1; eval_and_dump();
  if (++clk_cnt >= CLK_RATIO) { clk_cnt = 0; top->clock = !top->clock; }
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

    nvboard_bind_all_pins(top);
    nvboard_init(); 

    const char *vcd = getenv("NPC_TRACE");
    if (vcd != nullptr && *vcd != '\0') {
        const char *depth = getenv("NPC_TRACE_DEPTH");
        const char *cap = getenv("NPC_TRACE_CYCLES");
        trace_cycles = cap ? atoll(cap) : 100000;
        contextp->traceEverOn(true);
        tfp = new VerilatedVcdC;
        top->trace(tfp, depth ? atoi(depth) : 1); 
        tfp->open(vcd);
    }
    reset(100); 

    unsigned long long cycle = 0;
    long long inst_count = 0;

    for (; !contextp->gotFinish(); cycle++) {
        if (tfp && (long long)cycle >= trace_cycles) {
            tfp->close();
            delete tfp;
            tfp = nullptr;
            printf("[trace] capped at %lld cycles\n", trace_cycles);
        }
        nvboard_update();
        sim.cycle = cycle;
        sim.retired = false;
        single_cycle();

        if (sim.retired)
            inst_count++;
    }

    double ipc = (double)inst_count / (double)(cycle + 1);
    printf("Simulation stopped after %llu cycles, %lld instructions executed, IPC = %.4f (last pc = 0x%08x)\n",
           cycle + 1, inst_count, ipc, static_cast<unsigned>(sim.retire_pc));
    


    top->final();
    if (tfp) {
        tfp->close();
        delete tfp;
    }
    delete top;
    delete contextp;
    return 0;
}
