#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/Vsim_top.h"
#include "../obj_dir/Vsim_top___024root.h" // 访问 rootp 内部的 GPR 需要该类型的完整定义
#include "minirvemu.h"
#include "pmem.h"

#define PROGRAM_PATH "../../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin" // 缺省镜像
#define MAX_CYCLES 1e8

static VerilatedContext *contextp = nullptr;
static VerilatedVcdC *tfp = nullptr;
static Vsim_top *top = nullptr;

unsigned long long sim_cycle = 0; // 已仿真的周期数, 供外设把周期换算成时间

static void eval_and_dump()
{
    top->eval();
    contextp->timeInc(1);
    if (tfp) tfp->dump(contextp->time());
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
static int check_regs(const uint32_t *dut_regs, const uint32_t *ref_regs, int count)
{
    for (int i = 0; i < count; i++) {
        if (dut_regs[i] != ref_regs[i]) {
            //printf("r%d: dut=%u ref=%u\n", i, dut_regs[i], ref_regs[i]);
            printf("r%d: dut=0x%08x ref=0x%08x\n", i, dut_regs[i], ref_regs[i]);
            return 1;
        }
    }
    return 0;
}

static int check_pc(const uint32_t dut_pc, const uint32_t ref_pc)
{
    if (dut_pc != ref_pc) {
        printf("pc: dut=%u ref=%u\n", dut_pc, ref_pc);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    // 镜像路径可由命令行给出, 缺省用 PROGRAM_PATH
    const char *img = (argc > 1) ? argv[1] : PROGRAM_PATH;

    // REF 与 DUT 必须从同一份程序镜像的起始处开始执行
    int prog_size = ref_load_program(img);
    if (prog_size <= 0) {
        printf("reference failed to load program from %s\n", img);
        printf("Simulation stop\n");
        return 1;
    }

    // DUT 的存储器由 pmem.cpp 实现, 经 dpic_mem 通过 DPI-C 访问, 必须装入同一份程序镜像
    if (pmem_load(img) <= 0) {
        printf("failed to load program into pmem from %s\n", img);
        printf("Simulation stop\n");
        return 1;
    }
    ref_reset();

    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    top = new Vsim_top{contextp};

    // 默认不产生波形: train 跑满会写出几十 G 的 dump.vcd; 设 NPC_TRACE=<文件> 才开启
    const char *vcd = getenv("NPC_TRACE");
    if (vcd != nullptr && *vcd != '\0') {
        const char *depth = getenv("NPC_TRACE_DEPTH");
        contextp->traceEverOn(true);
        tfp = new VerilatedVcdC;
        top->trace(tfp, depth ? atoi(depth) : 1); // 默认只追踪顶层端口
        tfp->open(vcd);
    }
    reset(2); // DUT 复位后 PC=0x80000000, GPR 全0, 与 ref_reset() 的状态一致

    int cycle = 0;
    int finished = 0; 
    int failed = 0;   

    //for (; cycle < MAX_CYCLES && !contextp->gotFinish(); cycle++) {
    for (; !contextp->gotFinish(); cycle++) { // 无 MAX_CYCLES 限制
        sim_cycle = cycle;
        single_cycle();              // DUT 执行一条指令

        uint32_t *dut_regs = &top->rootp->sim_top__DOT__u_top__DOT__u_gpr__DOT__rf[0];
        uint32_t *ref_regs = ref_get_regs();

        if (top->misalign) {
            printf("NPC: lw/sw 地址未对齐, pc=%u\n", static_cast<unsigned>(top->pc));
            failed = 1;
            break;
        }

        if (top->ebreak) {
            printf("NPC hit ebreak\n");
            if (dut_regs[10] == 0) {
                printf("HIT GOOD TRAP\n");
                finished = 1;   // 只有 a0 == 0 才算成功结束
            } else {
                printf("HIT BAD TRAP: a0=%u\n", dut_regs[10]);
                failed = 1;     // 让 make 拿到非 0 退出码
            }
            break;
        }   
        

        if (ref_inst_cycle() != 0) { // REF 执行同一条指令
            printf("reference stopped on an invalid instruction\n");
            printf("Simulation stop\n");
            failed = 1;
            break;
        }


        // printf("cycle=%d pc=(dut=%u ref=%u) r0=%u r1=%u r2=%u r3=%u a0=%u a1=%u a2=%u a3=%u a4=%u\n",
        //        cycle, static_cast<unsigned>(top->pc),
        //        static_cast<unsigned>(ref_get_pc()),
        //        dut_regs[0], dut_regs[1], dut_regs[2], dut_regs[3],
        //        dut_regs[10], dut_regs[11], dut_regs[12], dut_regs[13], dut_regs[14]);

        if (check_regs(dut_regs, ref_regs, REF_REGISTER_COUNT)) {
            printf("pc = 0x%08x, inst = 0x%08x\n"
                   "GPR different\n",
                   static_cast<unsigned>(top->pc), static_cast<unsigned>(top->inst));
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
        printf("Difftest PASS: %d instructions executed, NPC == minirvEMU\n", cycle);
    else if (!failed)
        printf("Difftest: stopped after %d cycles, no ebreak (PC = 0x%08x)\n",
               cycle, static_cast<unsigned>(top->pc));

    top->final();
    if (tfp) {
        tfp->close();
        delete tfp;
    }
    delete top;
    delete contextp;
    return finished ? 0 : 1;
}
