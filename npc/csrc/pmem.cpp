#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/npc.h"

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
