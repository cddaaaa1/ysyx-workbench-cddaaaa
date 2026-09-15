#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/Vtop.h"
#include "../obj_dir/Vtop___024root.h" // 访问 rootp 内部的 GPR 需要该类型的完整定义
#include "../../semu/semu.h"

#define PROGRAM_PATH "program/sum.hex"
#define MAX_CYCLES 1000

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

// 逐字节比较 DUT 与 REF 的 GPR, 不一致时打印出错信息并返回非0
static int check_regs(const uint8_t *dut_regs, const uint8_t *ref_regs, int count)
{
    for (int i = 0; i < count; i++) {
        if (dut_regs[i] != ref_regs[i]) {
            printf("r%d: dut=%u ref=%u\n", i, dut_regs[i], ref_regs[i]);
            return 1;
        }
    }
    return 0;
}

static int check_pc(const uint8_t dut_pc, const uint8_t ref_pc)
{
    if (dut_pc != ref_pc) {
        printf("pc: dut=%u ref=%u\n", dut_pc, ref_pc);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    // REF 与 DUT 必须从同一份程序镜像的起始处开始执行
    int prog_size = ref_load_program(PROGRAM_PATH);
    if (prog_size <= 0) {
        printf("reference failed to load program from %s\n", PROGRAM_PATH);
        printf("Simulation stop\n");
        return 1;
    }
    ref_reset();

    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    contextp->traceEverOn(true);
    tfp = new VerilatedVcdC;
    top = new Vtop{contextp};
    top->trace(tfp, 0);
    tfp->open("dump.vcd");

    reset(2); // DUT 复位后 PC=0, GPR 全0, 与 ref_reset() 的状态一致

    int cycle = 0;
    int finished = 0; // 两边都执行完程序
    int failed = 0;   // 出错终止(非法指令)

    for (; cycle < MAX_CYCLES && !contextp->gotFinish(); cycle++) {
        if (ref_get_pc() >= prog_size) { // 程序执行完毕
            finished = 1;
            break;
        }

        single_cycle();              // DUT 执行一条指令
        if (ref_inst_cycle() != 0) { // REF 执行同一条指令
            printf("reference stopped on an invalid instruction\n");
            printf("Simulation stop\n");
            failed = 1;
            break;
        }

        uint8_t *dut_regs = &top->rootp->top__DOT__u_gpr__DOT__regs[0];
        uint8_t *ref_regs = ref_get_regs();


        printf("cycle=%d pc=(dut=%u ref=%u) r0=%u r1=%u r2=%u r3=%u\n",
               cycle, static_cast<unsigned>(top->pc),
               static_cast<unsigned>(ref_get_pc()),
               dut_regs[0], dut_regs[1], dut_regs[2], dut_regs[3]);

        if (check_regs(dut_regs, ref_regs, REF_REGISTER_COUNT)) {
            printf("GPR different\n");
            printf("Simulation stop\n");
            failed = 1;
            break;
        }
        if(check_pc(top->pc, ref_get_pc())) {
            printf("PC different\n");
            printf("Simulation stop\n");
            failed = 1;
            break;
        }
    }

    if (finished)
        printf("Difftest PASS: %d instructions executed, sCPU == sEMU\n", cycle);
    else if (!failed)
        printf("Difftest FAIL: stopped after %d cycles, program did not finish\n", cycle);

    top->final();
    tfp->close();
    delete top;
    delete tfp;
    delete contextp;
    return finished ? 0 : 1;
}