#include "minirvemu.h"

#include <stdio.h>
#include <string.h>

#define OP_IMM      0x13 // inst[6:0]   = 0010011, 立即数运算指令组
#define FUNCT3_ADDI 0x00 // inst[14:12] = 000,     该组内的 addi 指令

static uint32_t PC;
static uint32_t R[REF_REGISTER_COUNT];
static uint32_t M[REF_MEMORY_SIZE];
int ref_load_image(const uint32_t *insts, int count)
{
	if (insts == NULL || count <= 0 || count > REF_MEMORY_SIZE) //检查程序长度
		return -1;

	memset(M, 0, sizeof(M));
	for (int i = 0; i < count; i++)
		M[i] = insts[i];

	return count;
}

int ref_load_program(const char *path)
{
	FILE *program = fopen(path, "r");
	uint32_t insts[REF_MEMORY_SIZE];
	unsigned value;
	int count = 0;

	if (program == NULL) {
		perror(path);
		return -1;
	}

	while (count < REF_MEMORY_SIZE && fscanf(program, "%x", &value) == 1) //逐条读取指令
		insts[count++] = (uint32_t)value;

	fclose(program);
	if (count == 0) { //检查程序是否为空
		fprintf(stderr, "%s: no instruction loaded\n", path);
		return -1;
	}
	return ref_load_image(insts, count);
}

void ref_reset(void)
{
	PC = 0;
	memset(R, 0, sizeof(R));
}

int ref_inst_cycle(void)
{
	uint32_t addr = PC >> 2; 
	if (addr >= REF_MEMORY_SIZE) { // PC 出 M 的范围
		fprintf(stderr, "PC 0x%08x is out of memory range\n", PC);
		return -1;
	}

	// addi 的编码: imm[31:20] | rs1[19:15] | 000 | rd[11:7] | 0010011
	uint32_t inst   = M[addr];
	uint32_t opcode = inst & 0x7f;         
	uint32_t rd     = (inst >> 7) & 0x1f;  
	uint32_t funct3 = (inst >> 12) & 0x07; 
	uint32_t rs1    = (inst >> 15) & 0x1f; 
	int32_t  imm    = (int32_t)inst >> 20; 

	uint32_t next_pc = PC + 4; // RISC-V 指令位宽 4 字节, PC 按字节递增

	switch (opcode) {
	case OP_IMM: 
		switch (funct3) {
		case FUNCT3_ADDI:
			R[rd] = R[rs1] + (uint32_t)imm;
			break;
		default:
			fprintf(stderr, "invalid funct3 %u at PC=0x%08x\n", funct3, PC);
			return -1;
		}
		break;
	default:
		fprintf(stderr, "invalid opcode 0x%02x at PC=0x%08x\n", opcode, PC);
		return -1;
	}

	R[0] = 0; // x0 恒为 0
	PC = next_pc;
	return 0;
}

uint32_t *ref_get_regs(void)
{
	return R;
}

uint32_t ref_get_pc(void)
{
	return PC;
}





