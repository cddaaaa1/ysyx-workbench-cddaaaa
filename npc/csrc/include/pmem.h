#ifndef PMEM_H
#define PMEM_H
#include <stdint.h>


#define PMEM_BASE 0x80000000u
#define PMEM_SIZE 0x8000000
#define FLASH_BASE 0x30000000u
#define FLASH_SIZE  (16 * 1024 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t pmem[PMEM_SIZE];
extern uint8_t flash[FLASH_SIZE];

int  pmem_load(const char *path);
void flash_load(const char *path);

int  pmem_read(int raddr);
void pmem_write(int waddr, int wdata, char wmask);
void flash_read(int32_t raddr, int32_t *data);

#ifdef __cplusplus
}
#endif

#endif 