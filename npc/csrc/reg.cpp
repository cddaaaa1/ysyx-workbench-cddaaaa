#include <cinttypes>
#include <stdio.h>
#include "include/reg.h"
#include "include/sim.h"

static const char *const reg_names[] = {
  "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void reg_display() {
  const size_t gpr_count = sim_gpr_count();
  if (gpr_count == 0 || gpr_count > sizeof(reg_names) / sizeof(reg_names[0])) {
    puts("Register access is unavailable for this simulation.");
    return;
  }

  printf("%zu General Registers:\n", gpr_count);
  for (size_t i = 0; i < gpr_count; i++) {
    uint32_t value;
    if (!sim_read_gpr(i, value)) {
      printf("Failed to read x%zu.\n", i);
      return;
    }
    printf("%-4s: 0x%08" PRIx32, reg_names[i], value);
    putchar(i % 2 == 1 || i + 1 == gpr_count ? '\n' : '\t');
  }

  uint32_t pc;
  if (!sim_read_pc(pc)) {
    puts("Failed to read PC.");
    return;
  }
  printf("\nProgram Counter:\n");
  printf("%-3s: 0x%08" PRIx32 "\n", "pc", pc);
}
