#include <cstdlib>
#include <iostream>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/Vtop.h"

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static Vtop *top = nullptr;

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    tfp->dump(contextp->time());
}

static void single_cycle()
{
    top->clk = 0;
    eval_and_dump();
    top->clk = 1;
    eval_and_dump();
}

static void reset(int cycles)
{
    top->rst = 1;
    while (cycles-- > 0)
        single_cycle();
    top->rst = 0;
}

int main(int argc, char **argv)
{
    int cycles = 32;

    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    contextp->traceEverOn(true);
    tfp = new VerilatedVcdC;
    top = new Vtop{contextp};
    top->trace(tfp, 0);
    tfp->open("dump.vcd");

    reset(2);
    for (int cycle = 0; cycle < cycles && !contextp-> gotFinish(); cycle++) {
        single_cycle();
        std::cout << "cycle=" << cycle
                  << " pc=" << static_cast<unsigned>(top->pc)
                  << " r0=" << static_cast<unsigned>(top->r0)
                  << " r1=" << static_cast<unsigned>(top->r1)
                  << " r2=" << static_cast<unsigned>(top->r2)
                  << " r3=" << static_cast<unsigned>(top->r3)
                  << '\n';
    }

    top->final();
    tfp->close();
    delete top;
    delete tfp;
    delete contextp;
    return 0;
}