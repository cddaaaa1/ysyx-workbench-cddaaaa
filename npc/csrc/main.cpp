#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/VSimTop.h"
#include "minirvemu.h"
#include "pmem.h"

#define PROGRAM_PATH "../../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin" // 缺省镜像
#define MAX_CYCLES 1e8

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static VSimTop *top = nullptr;

unsigned long long sim_cycle = 0; // 已仿真的周期数, 供外设把周期换算成时间

static bool g_retired = false;
static uint32_t g_retire_pc = 0, g_retire_inst = 0;

extern "C" void flash_read(int32_t addr, int32_t *data) { assert(0); }

extern "C" void sim_retire(int pc, int inst)
{
    g_retire_pc   = static_cast<uint32_t>(pc);
    g_retire_inst = static_cast<uint32_t>(inst);
    g_retired = true;
}

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    if (tfp) tfp->dump(contextp->time());
}

void single_cycle() {
  top->clock = 0; top->cpuClock = 0; top->eval();
  top->clock = 1; top->cpuClock = 1; top->eval();
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
    long long inst_count = 0; // 已执行完毕的指令数, 用于测量 IPC

    for (; !contextp->gotFinish(); cycle++) {
        sim_cycle = cycle;
        g_retired = false;
        single_cycle();              // DUT 走一个周期

        // RTL 在指令退休的那一刻通过 DPI-C 回调 sim_retire
        // (SoC 流程下仿真环境看不到 pc / ebreak, 所以这里只统计数据, 不做 difftest)
        if (g_retired)
            inst_count++;
    }

    printf("Simulation stopped after %llu cycles, %lld instructions executed (last pc = 0x%08x)\n",
           cycle + 1, inst_count, static_cast<unsigned>(g_retire_pc));

    top->final();
    if (tfp) {
        tfp->close();
        delete tfp;
    }
    delete top;
    delete contextp;
    return 0;
}
