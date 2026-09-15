#include "semu.h"

#include <stdio.h>
#include <string.h>

#define OP_ADD   0x00
#define OP_LI    0x80
#define OP_BNER0 0xc0

static uint8_t PC;
static uint8_t R[REF_REGISTER_COUNT];
static uint8_t M[REF_MEMORY_SIZE];

int ref_load_program(const char *path)
{
	FILE *program = fopen(path, "r");
	unsigned value;
	int address = 0;

	if (program == NULL) {
		perror(path);
		return -1;
	}

	memset(M, 0, sizeof(M));
	while (address < REF_MEMORY_SIZE && fscanf(program, "%x", &value) == 1) //逐条读取指令
		M[address++] = (uint8_t)value;

	fclose(program);
	if (address == 0) { //检查程序是否为空
		fprintf(stderr, "%s: no instruction loaded\n", path);
		return -1;
	}
	return address;
}

void ref_reset(void)
{
	PC = 0;
	memset(R, 0, sizeof(R));
}

int ref_inst_cycle(void)
{
	uint8_t inst = M[PC];
	uint8_t opcode = inst & 0xc0;
	uint8_t rd = (inst >> 4) & 0x03;
	uint8_t rs1 = (inst >> 2) & 0x03;
	uint8_t add_rs2 = inst & 0x03;
	uint8_t branch_rs2 = (inst >> 1) & 0x03;
	uint8_t next_pc = PC + 1;

	switch (opcode) {
	case OP_ADD:
		R[rd] = R[rs1] + R[add_rs2];
		break;
	case OP_LI:
		R[rd] = inst & 0x0f;
		break;
	case OP_BNER0:
		if (R[0] != R[branch_rs2])
			next_pc = (inst >> 3) & 0x07;
		break;
	default:
		fprintf(stderr, "invalid sISA opcode 0x%02x at PC=%u\n", opcode, PC);
		return -1;
	}

	PC = next_pc;
	return 0;
}

uint8_t *ref_get_regs(void)
{
	return R;
}

uint8_t ref_get_pc(void)
{
	return PC;
}
