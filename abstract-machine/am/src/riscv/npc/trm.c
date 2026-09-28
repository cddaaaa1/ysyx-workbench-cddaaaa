#include <am.h>
#include <klib-macros.h>

#define UART16550            0x10000000
#define UART_REG_TX          (UART16550 + 0x0)
#define UART_REG_LC          (UART16550 + 0x3)
#define UART_REG_DL1         (UART16550 + 0x0) //LSB
#define UART_REG_DL2         (UART16550 + 0x1) //MSB
#define UART_REG_LS          (UART16550 + 0x5)

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
  while ((*(volatile uint8_t *)UART_REG_LS & 0x20) == 0) ;  // 等发送队列可接收 (bit5)
  *(volatile uint8_t *)UART_REG_TX = ch;                // 写 THR，推入发送队列
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));
  while (1);
}

static void uart_init(uint32_t baud_rate) {
  // uint16_t divisor = 2.5e7/(16 * baud_rate);
  uint16_t divisor = 13; 
  *(volatile uint8_t *)UART_REG_LC = 0x83;  // LCR: DLAB=1, 8 数据位，1 停止位，无校验；
  *(volatile uint8_t *)UART_REG_DL1 = divisor;         // DLL: 除数低8位
  *(volatile uint8_t *)UART_REG_DL2 = divisor >> 8;    // DLM: 除数高8位 
  *(volatile uint8_t *)UART_REG_LC = 0x03;  // LCR: DLAB=0, 8 数据位，1 停止位，无校验；
}

void _trm_init() {
  uart_init(115200);
  int ret = main(mainargs);
  halt(ret);
}
