#include <cstdint>
#include <cstdio>
#include "../obj_dir/Vtop.h"

#ifdef NVBOARD_SIM

#include "verilated.h"
#include "verilated_vcd_c.h"

VerilatedContext* contextp = NULL;
VerilatedVcdC* tfp = NULL;
static Vtop* top;

static int error_count = 0;

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

static void check(int ctrl, int a, int b,
                  uint8_t expected_out, bool expected_of,
                  bool expected_cf, bool expected_zf) {
  uint8_t got_out = (uint8_t)(top->ledr & 0xf);
  bool got_of = ((top->ledr >> 4) & 1) != 0;
  bool got_cf = ((top->ledr >> 5) & 1) != 0;
  bool got_zf = ((top->ledr >> 6) & 1) != 0;

  if (got_out != expected_out || got_of != expected_of ||
      got_cf != expected_cf || got_zf != expected_zf) {
    printf("Error: ctrl=%d a=%d b=%d -> expected out=%d of=%d cf=%d zf=%d, "
           "got out=%d of=%d cf=%d zf=%d\n",
           ctrl, a, b, expected_out, expected_of, expected_cf, expected_zf,
           got_out, got_of, got_cf, got_zf);
    error_count++;
  }
}

int main() {
  sim_init();

  for (int ctrl = 0; ctrl < 8; ctrl++) {
    for (int i = -8; i <= 7; i++) {
      for (int j = -8; j <= 7; j++) {
        // sw[3:0] = a, sw[7:4] = b.
        top->sw = ((uint8_t)(j & 0xf) << 4) | (uint8_t)(i & 0xf);
        top->btn = ctrl;
        step_and_dump_wave();

        uint8_t a4 = (uint8_t)i & 0xf;
        uint8_t b4 = (uint8_t)j & 0xf;
        uint8_t expected_out = 0;
        bool expected_of = false;
        bool expected_cf = false;

        switch (ctrl) {
          case 0: {  // add
            int s = i + j;
            expected_out = (uint8_t)(s & 0xf);
            expected_of = (s > 7) || (s < -8);
            expected_cf = ((a4 + b4) >> 4) & 1;
            break;
          }
          case 1: {  // sub
            int d = i - j;
            expected_out = (uint8_t)(d & 0xf);
            expected_of = (d > 7) || (d < -8);
            expected_cf = ((a4 + ((~b4) & 0xf) + 1) >> 4) & 1;
            break;
          }
          case 2: expected_out = (~a4) & 0xf; break;         // not
          case 3: expected_out = (a4 & b4) & 0xf; break;     // and
          case 4: expected_out = (a4 | b4) & 0xf; break;     // or
          case 5: expected_out = (a4 ^ b4) & 0xf; break;     // xor
          case 6: expected_out = (i < j) ? 1 : 0; break;     // signed lt
          case 7: expected_out = (i == j) ? 1 : 0; break;    // equal
        }

        bool expected_zf = (expected_out == 0);

        check(ctrl, i, j, expected_out, expected_of, expected_cf, expected_zf);
      }
    }
  }

  sim_exit();

  if (error_count == 0) {
    printf("ALL TESTS PASSED\n");
    return 0;
  }
  printf("TEST FAILED: %d errors\n", error_count);
  return 1;
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
