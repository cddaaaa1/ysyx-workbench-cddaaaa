#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pmem.h"

static uint8_t pmem[PMEM_SIZE];

// 检查 [addr, addr+4) 是否落在 pmem 范围内 (addr 为绝对地址)
static int addr_valid(uint32_t addr)
{
	if (addr < PMEM_BASE || addr >= PMEM_BASE + PMEM_SIZE) {
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