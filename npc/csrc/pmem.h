#ifndef PMEM_H
#define PMEM_H


#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000 

int pmem_load(const char *path);

uint32_t pmem_uart_status(void);
uint32_t pmem_rtc_lo(void);
uint32_t pmem_rtc_hi(void);

#define FLASH_BASE 0x30000000u
#define FLASH_SIZE  (16 * 1024 * 1024)

// extern uint8_t flash[FLASH_SIZE];

int flash_load(const char *path);

#endif
