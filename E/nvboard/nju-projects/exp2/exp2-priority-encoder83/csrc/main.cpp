#include "../obj_dir/Vtop.h"

#ifdef NVBOARD_SIM
#include "verilated.h"
#include "verilated_types.h"
#include "verilated_vcd_c.h"

VerilatedContext* contextp = NULL;
VerilatedVcdC* tfp = NULL;
static Vtop* top;

static void step_and_dump_wave() {
  top->eval();
  contextp->timeInc(1);
  tfp->dump(contextp->time());
}

static void sim_init() {
  contextp = new VerilatedContext;
  tfp = new VerilatedVcdC;
  top = new Vtop;
  contextp->traceEverOn(true);
  top->trace(tfp, 0);
  tfp->open("dump.vcd");
}
static void sim_exit() {
  step_and_dump_wave();
  tfp->close();
}

int main() {
  sim_init();
  for (int input = 0; input < 256; input++) {
    top->sw = input;
    step_and_dump_wave();
  }
  sim_exit();
}
#else
#include <nvboard.h>

static Vtop dut;

void nvboard_bind_all_pins(TOP_NAME* top);

int main() {
  nvboard_bind_all_pins(&dut);
  nvboard_init();

  while (true) {
    nvboard_update();
    dut.eval();
  }
}
#endif
