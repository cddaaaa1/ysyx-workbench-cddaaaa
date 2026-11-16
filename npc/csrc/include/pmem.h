#ifndef PMEM_H
#define PMEM_H
#include <stdint.h>


#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000
#define FLASH_BASE 0x30000000u
#define FLASH_SIZE  (16 * 1024 * 1024)

extern uint8_t pmem[PMEM_SIZE];
extern uint8_t flash[FLASH_SIZE];

uint8_t* guest_to_host(uint32_t paddr);

extern "C" int  pmem_load(const char *path);
extern "C" void flash_load(const char *path);
extern "C" uint32_t pmem_read(uint32_t raddr);
extern "C" void pmem_write(uint32_t waddr, uint32_t wdata, char wmask);
extern "C" void flash_read(int32_t raddr, int32_t *data);

#endif
