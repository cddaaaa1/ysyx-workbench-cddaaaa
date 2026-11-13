#ifndef NPC_DIFFTEST_H
#define NPC_DIFFTEST_H
#include <stdint.h>
#include <stddef.h>

typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
enum {
  DIFFTEST_TO_DUT = 0,
  DIFFTEST_TO_REF = 1
};

void init_difftest(const char *ref_so_file, long img_size, int port);
void difftest_step(vaddr_t pc, vaddr_t npc);
#endif
