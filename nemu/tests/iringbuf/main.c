#include <am.h>

int main(const char *args) {
  volatile uint32_t count = 0;
  for (uint32_t i = 0; i < 32; i ++) {
    count ++;
  }

  if (args[0] == 'a') {
    // 当前 NEMU 尚未实现 ecall，触发 NEMU_ABORT。
    asm volatile("ecall");
  } else {
    // 当前物理内存范围为 [0x80000000, 0x87ffffff]。
    *(volatile uint32_t *)0x90000000u = count;
  }

  return 1;
}
