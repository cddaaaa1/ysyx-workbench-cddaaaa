#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/npc.h"

#define UART_ADDR        0x10000000u
#define UART_STATUS_ADDR 0x10000004u
#define RTC_ADDR         0x20000000u
#define RTC_ADDR_HI      0x20000004u
#define NPC_FREQ_HZ      4e8

static uint32_t g_retire_inst = 0;
static uint32_t g_uart_status = 0;
static uint32_t g_rtc_lo = 0, g_rtc_hi = 0;

uint32_t pmem_uart_status(void) { return g_uart_status; }
uint32_t pmem_rtc_lo(void) { return g_rtc_lo; }
uint32_t pmem_rtc_hi(void) { return g_rtc_hi; }

static uint64_t get_time_us() {
    return (uint64_t)(sim.cycle / NPC_FREQ_HZ * 1000000.0);
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

extern "C" void sim_retire(int pc, int inst)
{
	sim.retire_pc   = (uint32_t)pc;
	g_retire_inst = (uint32_t)inst;
	sim.retired = true;
}

extern "C" int pmem_read(int raddr)
{
	uint32_t addr = (uint32_t)raddr & ~0x3u;
	if (raddr == UART_STATUS_ADDR) {
		g_uart_status = (rand() & 0x7) == 0 ? 1 : 0;
		return g_uart_status;
	}
	else if (raddr == RTC_ADDR) {
		g_rtc_lo = (uint32_t)(get_time_us() & 0xffffffff);
		return g_rtc_lo;
	}
	else if (raddr == RTC_ADDR_HI) {
		g_rtc_hi = (uint32_t)(get_time_us() >> 32);
		return g_rtc_hi;
	}

	if (!addr_valid(addr))
		return 0;

	uint32_t off = addr - PMEM_BASE;
	return (int)((uint32_t)pmem[off + 0]
	           | (uint32_t)pmem[off + 1] << 8
	           | (uint32_t)pmem[off + 2] << 16
	           | (uint32_t)pmem[off + 3] << 24);
}

extern "C" void pmem_write(int waddr, int wdata, char wmask)
{
	if (waddr == UART_ADDR) {
		fputc(wdata & 0xff, stderr);
		return;
	}
	uint32_t addr = (uint32_t)waddr & ~0x3u;
	if (!addr_valid(addr))
		return;
	uint32_t off = addr - PMEM_BASE;

	if (wmask & 0x1) pmem[off]     = ((uint32_t)wdata & 0xff);
	if (wmask & 0x2) pmem[off + 1] = ((uint32_t)wdata >> 8) & 0xff;
	if (wmask & 0x4) pmem[off + 2] = ((uint32_t)wdata >> 16) & 0xff;
	if (wmask & 0x8) pmem[off + 3] = ((uint32_t)wdata >> 24) & 0xff;
}

extern "C" void flash_read(int32_t raddr, int32_t *data)
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
