#include <stdint.h>
#include <stdio.h>
#include "pmem.h"

static uint8_t pmem[PMEM_SIZE];

// 检查 [addr, addr+4) 是否落在 pmem 范围内
static int addr_valid(uint32_t addr)
{
	if (addr + 4 > PMEM_SIZE) {
		fprintf(stderr, "[pmem] access out of range: 0x%08x (size = 0x%x)\n",
		        addr, PMEM_SIZE);
		return 0;
	}
	return 1;
}

// ---- 供 RTL 通过 DPI-C 调用的接口 ---
extern "C" int pmem_read(int raddr)
{
	uint32_t addr = (uint32_t)raddr & ~0x3u; // 只支持按 4 字节对齐的读
	if (!addr_valid(addr))
		return 0;

	// RISC-V 是小端: 低地址存放低字节
	return (int)((uint32_t)pmem[addr + 0]
	           | (uint32_t)pmem[addr + 1] << 8
	           | (uint32_t)pmem[addr + 2] << 16
	           | (uint32_t)pmem[addr + 3] << 24);
}


// 总是往地址为`waddr & ~0x3u`的4字节按写掩码`wmask`写入`wdata`
// `wmask`中每比特表示`wdata`中1个字节的掩码
extern "C" void pmem_write(int waddr, int wdata, char wmask)
{
	uint32_t addr = (uint32_t)waddr & ~0x3u;
	if (!addr_valid(addr))
		return;
	
    if (wmask & 0x1) pmem[addr] = ((uint32_t)wdata & 0xff);
    if (wmask & 0x2) pmem[addr + 1] = ((uint32_t)wdata >> 8) & 0xff;
    if (wmask & 0x4) pmem[addr + 2] = ((uint32_t)wdata >> 16) & 0xff;
    if (wmask & 0x8) pmem[addr + 3] = ((uint32_t)wdata >> 24) & 0xff;
}

// ---- 程序装入 ----
int pmem_load(const char *path)
{
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		perror(path);
		return -1;
	}

	unsigned value;
	int count = 0;
	while (fscanf(fp, "%x", &value) == 1) { // 逐行读一个 32 位字
		if ((uint32_t)(count + 1) * 4 > (uint32_t)PMEM_SIZE) {
			fprintf(stderr, "[pmem] %s is too large for 0x%x bytes\n", path, (unsigned)PMEM_SIZE);
			fclose(fp);
			return -1;
		}
		// 复用 pmem_write, 顺便走一遍"按掩码写字节"的通路
		pmem_write(count * 4, (int)value, 0xf);
		count++;
	}

	fclose(fp);
	if (count == 0) {
		fprintf(stderr, "%s: no instruction loaded\n", path);
		return -1;
	}
	printf("[pmem] %s: %d words (0x%x bytes) loaded\n", path, count, count * 4);
	return count;
}