#include <cstdint>
#include <cstdio>
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "../obj_dir/Vtop.h"

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

// Sign-extend a 4-bit two's complement value to int8_t.
static int8_t sign_extend4(uint8_t v) {
  v &= 0xf;
  return (v & 0x8) ? (int8_t)(v | 0xf0) : (int8_t)v;
}

// Equivalent of the `task check` in the lecture notes: compare the DUT
// outputs with the expected values and report any mismatch.
static void check(const char* op, int a, int b,
                  uint8_t expected_s, bool expected_of,
                  bool expected_cf, bool expected_zf) {
  uint8_t got_s = (uint8_t)(top->s & 0xf);
  bool got_of = (top->of != 0);
  bool got_cf = (top->cf != 0);
  bool got_zf = (top->zf != 0);

  if (got_s != expected_s || got_of != expected_of ||
      got_cf != expected_cf || got_zf != expected_zf) {
    printf("Error: %s a=%d b=%d -> expected s=%d of=%d cf=%d zf=%d, "
           "got s=%d of=%d cf=%d zf=%d\n",
           op, a, b, sign_extend4(expected_s), expected_of, expected_cf,
           expected_zf, sign_extend4(got_s), got_of, got_cf, got_zf);
    error_count++;
  }
}

int main() {
  sim_init();

  // Addition: a + b, exhaustive over the full two's complement range.
  for (int i = -8; i <= 7; i++) {
    for (int j = -8; j <= 7; j++) {
      top->a = (uint8_t)i & 0xf;
      top->b = (uint8_t)j & 0xf;
      top->sub = 0;
      step_and_dump_wave();

      int sum = i + j;
      uint8_t expected_s = (uint8_t)(sum & 0xf);
      bool expected_of = (sum > 7) || (sum < -8);
      bool expected_cf = ((((uint8_t)i & 0xf) + ((uint8_t)j & 0xf)) >> 4) & 1;
      bool expected_zf = (expected_s == 0);

      check("add", i, j, expected_s, expected_of, expected_cf, expected_zf);
    }
  }

  // Subtraction: a - b, exhaustive over the full two's complement range.
  for (int i = -8; i <= 7; i++) {
    for (int j = -8; j <= 7; j++) {
      top->a = (uint8_t)i & 0xf;
      top->b = (uint8_t)j & 0xf;
      top->sub = 1;
      step_and_dump_wave();

      int diff = i - j;
      uint8_t expected_s = (uint8_t)(diff & 0xf);
      bool expected_of = (diff > 7) || (diff < -8);
      bool expected_cf = ((((uint8_t)i & 0xf) + ((~(uint8_t)j) & 0xf) + 1) >> 4) & 1;
      bool expected_zf = (expected_s == 0);

      check("sub", i, j, expected_s, expected_of, expected_cf, expected_zf);
    }
  }

  sim_exit();

  if (error_count == 0) {
    printf("ALL TESTS PASSED\n");
    return 0;
  } else {
    printf("TEST FAILED: %d errors\n", error_count);
    return 1;
  }
}
