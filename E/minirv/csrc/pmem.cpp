#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmem.h"
#include "sys/time.h"

#define UART_ADDR 0x10000000u // 串口输出寄存器, AM 的 putch 往这里写
#define UART_STATUS_ADDR 0x10000004u


static uint8_t pmem[PMEM_SIZE];

// 设备读出值的暂存: DUT 读设备时写入, 参考模型通过下面的 getter 读取
static uint32_t g_uart_status = 0;
static uint32_t g_rtc_lo = 0, g_rtc_hi = 0;

uint32_t pmem_uart_status(void) { return g_uart_status; }
uint32_t pmem_rtc_lo(void) { return g_rtc_lo; }
uint32_t pmem_rtc_hi(void) { return g_rtc_hi; }

static uint64_t get_time_us() {
    static uint64_t start = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    if (start == 0)
        start = now;      
    return now - start;
}

// 检查 [addr, addr+4) 是否落在 pmem 范围内 (addr 为绝对地址)
static int addr_valid(uint32_t addr)
{
	// 低于基址: 复位期间 DUT 会拿一个未就绪的 pc 去取指(读到 0), 这类访问直接忽略
	if (addr < PMEM_BASE)
		return 0;
	if (addr >= PMEM_BASE + PMEM_SIZE) {
		fprintf(stderr, "[pmem] access out of range: 0x%08x (base = 0x%08x, size = 0x%x)\n",
		        addr, PMEM_BASE, PMEM_SIZE);
		return 0;
	}
	return 1;
}

// ---- 供 RTL 通过 DPI-C 调用的接口 ---
extern "C" int pmem_read(int raddr)
{
	uint32_t addr = (uint32_t)raddr & ~0x3u; // 只支持按 4 字节对齐的读
	if (raddr == UART_STATUS_ADDR) {
        g_uart_status = (rand() & 0x7) == 0 ? 1 : 0;
        return g_uart_status;
    }
	else if (raddr == 0x20000000) {
            g_rtc_lo = (uint32_t)(get_time_us() & 0xffffffff);
            return g_rtc_lo;
    }
    else if (raddr == 0x20000004) { 
            g_rtc_hi = (uint32_t)(get_time_us() >> 32);
            return g_rtc_hi;
    }

	if (!addr_valid(addr))
		return 0;

	uint32_t off = addr - PMEM_BASE; // 相对基址的偏移才是数组下标
	// RISC-V 是小端: 低地址存放低字节
	return (int)((uint32_t)pmem[off + 0]
	           | (uint32_t)pmem[off + 1] << 8
	           | (uint32_t)pmem[off + 2] << 16
	           | (uint32_t)pmem[off + 3] << 24);
}


// 总是往地址为`waddr & ~0x3u`的4字节按写掩码`wmask`写入`wdata`
// `wmask`中每比特表示`wdata`中1个字节的掩码
extern "C" void pmem_write(int waddr, int wdata, char wmask)
{
	if (waddr == UART_ADDR) {  // 写入UART
		fputc(wdata & 0xff, stderr);   // 在stdio.h中定义
		return;
	}

	uint32_t addr = (uint32_t)waddr & ~0x3u ;
	if (!addr_valid(addr))
		return;

	uint32_t off = addr - PMEM_BASE; // 相对基址的偏移才是数组下标

    if (wmask & 0x1) pmem[off]     = ((uint32_t)wdata & 0xff);
    if (wmask & 0x2) pmem[off + 1] = ((uint32_t)wdata >> 8) & 0xff;
    if (wmask & 0x4) pmem[off + 2] = ((uint32_t)wdata >> 16) & 0xff;
    if (wmask & 0x8) pmem[off + 3] = ((uint32_t)wdata >> 24) & 0xff;
}

// ---- 程序装入 ----
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