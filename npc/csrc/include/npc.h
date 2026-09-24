#ifndef NPC_H
#define NPC_H

#include <stdint.h>

// ---- 存储 ----
#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000

#define FLASH_BASE 0x30000000u
#define FLASH_SIZE  (16 * 1024 * 1024)

extern uint8_t pmem[PMEM_SIZE];
extern uint8_t flash[FLASH_SIZE];

int pmem_load(const char *path);
void flash_load(const char *path);

uint32_t pmem_uart_status(void);
uint32_t pmem_rtc_lo(void);
uint32_t pmem_rtc_hi(void);

struct SimState {
	unsigned long long cycle = 0;      // 当前周期数, RTC 用
	bool               retired = false; // 本拍提交了指令
	uint32_t           retire_pc = 0;
};

extern SimState sim;

#endif
