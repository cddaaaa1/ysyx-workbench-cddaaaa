#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/pmem.h"

#define UART16550    0x10000000
#define UART_REG_TX  (UART16550 + 0x0) 
#define UART_REG_LC  (UART16550 + 0x3) 
#define UART_REG_LS  (UART16550 + 0x5) 
#define UART_REG_MASK 0x7          
#define UART_LS_TFE  0x20             
#define UART_LC_DLAB 0x80             

static uint8_t g_uart_lc = 0; 

uint8_t pmem[PMEM_SIZE];
uint8_t flash[FLASH_SIZE];

int pmem_load(const char *path)
{
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) {
		perror(path);
		return -1;
	}
	memset(pmem, 0, sizeof(pmem));
	size_t size = fread(pmem, 1, sizeof(pmem), fp);
	fclose(fp);

	if (size == 0) {
		fprintf(stderr, "%s: no instruction loaded\n", path);
		return -1;
	}
	printf("[pmem] %s: %u bytes loaded at 0x%08x\n", path, (unsigned)size, PMEM_BASE);
	return (int)size;
}

void flash_load(const char *path)
{
	FILE *fp = fopen(path, "rb");
	assert(fp != NULL);

	memset(flash, 0, sizeof(flash));
	size_t size = fread(flash, 1, sizeof(flash), fp);
	fclose(fp);

	assert(size > 0);
	printf("[flash] %s: %u bytes loaded at 0x%08x\n", path, (unsigned)size, FLASH_BASE);
}

static int addr_valid(uint32_t addr)
{
	if (addr < PMEM_BASE)
		return 0;
	if (addr >= PMEM_BASE + PMEM_SIZE) {
		fprintf(stderr, "[pmem] access out of range: 0x%08x (base = 0x%08x, size = 0x%x)\n",
		        addr, PMEM_BASE, PMEM_SIZE);
		return 0;
	}
	return 1;
}

int pmem_read(int raddr)
{
	uint32_t raw = (uint32_t)raddr;
	
	if ((raw & ~UART_REG_MASK) == UART16550) {
		uint32_t v = (raw == UART_REG_LS) ? UART_LS_TFE : 0;
		return (int)(v << ((raw & 0x3u) * 8u));
	}

	uint32_t addr = raw & ~0x3u;
	if (!addr_valid(addr))
		return 0;

	uint32_t off = addr - PMEM_BASE;
	return (int)((uint32_t)pmem[off + 0]
	           | (uint32_t)pmem[off + 1] << 8
	           | (uint32_t)pmem[off + 2] << 16
	           | (uint32_t)pmem[off + 3] << 24);
}

void pmem_write(int waddr, int wdata, char wmask)
{
	uint32_t raw = (uint32_t)waddr;

	if ((raw & ~UART_REG_MASK) == UART16550) {
		uint8_t b = (uint8_t)((uint32_t)wdata >> ((raw & 0x3u) * 8u));
		if      (raw == UART_REG_LC)                            g_uart_lc = b;
		else if (raw == UART_REG_TX && !(g_uart_lc & UART_LC_DLAB)) fputc(b, stderr);
		return;
	}

	uint32_t addr = raw & ~0x3u;
	if (!addr_valid(addr))
		return;
	uint32_t off = addr - PMEM_BASE;

	if (wmask & 0x1) pmem[off]     = ((uint32_t)wdata & 0xff);
	if (wmask & 0x2) pmem[off + 1] = ((uint32_t)wdata >> 8) & 0xff;
	if (wmask & 0x4) pmem[off + 2] = ((uint32_t)wdata >> 16) & 0xff;
	if (wmask & 0x8) pmem[off + 3] = ((uint32_t)wdata >> 24) & 0xff;
}

void flash_read(int32_t raddr, int32_t *data)
{
	uint32_t off = (uint32_t)raddr & 0xFFFFFFu;
	off &= ~0x3u;

	if (off + 4 > FLASH_SIZE) {
		fprintf(stderr, "[flash] read out of range: off = 0x%08x\n", off);
		*data = 0;
		return;
	}

	uint32_t temp = (uint32_t)flash[off + 0]
	              | (uint32_t)flash[off + 1] << 8
	              | (uint32_t)flash[off + 2] << 16
	              | (uint32_t)flash[off + 3] << 24;

	*data = (int32_t)temp;
}
