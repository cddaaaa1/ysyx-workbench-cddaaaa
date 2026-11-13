#ifdef DIFFTEST
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include "include/pmem.h"
#include "include/difftest.h"
#include "include/reg.h"
#include "include/sim.h"
#include "include/utils.h"

void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *, bool) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;

void init_difftest(const char *ref_so_file, long img_size, int port) {
  assert(ref_so_file != NULL);

  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY);
  assert(handle);

  ref_difftest_memcpy = (void (*)(paddr_t, void *, size_t, bool))dlsym(handle, "difftest_memcpy");
  assert(ref_difftest_memcpy);

  ref_difftest_exec = (void (*)(uint64_t))dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  ref_difftest_regcpy = (void (*)(void *dut, bool direction))dlsym(handle, "difftest_regcpy");
  assert(ref_difftest_regcpy);

  void (*ref_difftest_init)(int) = (void (*)(int))dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

  printf("[difftest] REF: %s\n", ref_so_file);

  ref_difftest_init(port);
  ref_difftest_memcpy(PMEM_BASE, guest_to_host(PMEM_BASE), img_size, DIFFTEST_TO_REF);

  for (int i = 0; i < 32; i++) {
    sim_read_gpr(i, cpu_state.gpr[i]);
  }
  sim_read_pc(cpu_state.pc);
  ref_difftest_regcpy(&cpu_state, DIFFTEST_TO_REF);
}

static bool difftest_checkregs(CPUState *ref_r, vaddr_t pc) {
  if (ref_r->pc != cpu_state.pc) {
    printf("Difftest: pc is different at pc = 0x%08x, ref.pc = 0x%08x, dut.pc = 0x%08x\n",
           pc, ref_r->pc, cpu_state.pc);
    return false;
  }
  size_t gpr_count = sizeof(cpu_state.gpr) / sizeof(cpu_state.gpr[0]);
  for (size_t i = 0; i < gpr_count; i++) {
    if (ref_r->gpr[i] != cpu_state.gpr[i]) {
      printf("Difftest: gpr[%zu] is different at pc = 0x%08x, ref.gpr[%zu] = 0x%08x, dut.gpr[%zu] = 0x%08x\n",
             i, pc, i, ref_r->gpr[i], i, cpu_state.gpr[i]);
      return false;
    }
  }
  return true;
}

static void checkregs(CPUState *ref, vaddr_t pc) {
  if (!difftest_checkregs(ref, pc)) {
    npc_state.state = NPC_ABORT;
    npc_state.halt_pc = pc;
    reg_display();
  }
}

void difftest_step(vaddr_t pc, vaddr_t npc) {
  CPUState ref_r;
  //TODO: skip_dut_nr_instr
  //TODO: is_skip_ref

  for (int i = 0; i < 32; i++) {
    sim_read_gpr(i, cpu_state.gpr[i]);
  }
  cpu_state.pc = npc;

  ref_difftest_exec(1);
  ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
  checkregs(&ref_r, pc);
}
#endif 