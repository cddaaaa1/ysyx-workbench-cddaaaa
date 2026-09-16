#include "minirvemu.h"

#include <stdio.h>
#include <string.h>

// opcode 组 (inst[6:0]), 组名取自 RISC-V 手册的 opcode 编码表
#define OP_IMM      0x13 // 0010011, 立即数运算组 (OP-IMM)
#define OP_JALR     0x67 // 1100111, 跳转并链接组 (JALR)
#define OP_R        0x33 // 0110011, 寄存器-寄存器运算组 (OP)
#define OP_LUI      0x37 // 0110111, 大立即数组 (LUI, U 型)

// funct3 (inst[14:12]) 与 funct7 (inst[31:25]), 在同一 opcode 组内区分具体指令
#define FUNCT3_ADDI 0x00 // OP_IMM  组内的 addi
#define FUNCT3_JALR 0x00 // OP_JALR 组内的 jalr
#define FUNCT3_ADD  0x00 // OP_R    组内的 add
#define FUNCT7_ADD  0x00 // OP_R    组内的 add (sub 的 funct7 为 0x20)

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

	// I 型: imm[31:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | opcode[6:0]
	// R 型: funct7[31:25] | rs2[24:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | opcode[6:0]
	// U 型: imm[31:12] | rd[11:7] | opcode[6:0] 
	// 这里把用得到的字段统一取出来, 各指令组按自己的格式取用
	uint32_t inst   = M[addr];
	uint32_t opcode = inst & 0x7f;         
	uint32_t rd     = (inst >> 7) & 0x1f;  
	uint32_t funct3 = (inst >> 12) & 0x07; 
	uint32_t rs1    = (inst >> 15) & 0x1f; 
	uint32_t rs2    = (inst >> 20) & 0x1f;
	int32_t  imm    = (int32_t)inst >> 20;
	uint32_t funct7 = (inst >> 25) & 0x7f;
	
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
	case OP_JALR:
		switch (funct3) {
		case FUNCT3_JALR: {
			// 先把目标算出来再写 rd: rd 和 rs1 允许是同一个寄存器
			uint32_t target = (R[rs1] + (uint32_t)imm) & ~1u; // 目标最低位清零
			R[rd] = PC + 4; // 链接地址 = 下一条指令的地址
			next_pc = target;
			break;
		}
		default:
			fprintf(stderr, "invalid funct3 %u at PC=0x%08x\n", funct3, PC);
			return -1;
		}
		break;
	case OP_R: 
		switch (funct3) {
		case FUNCT3_ADD:
			switch (funct7) {
			case FUNCT7_ADD: 
				R[rd] = R[rs1] + R[rs2];
				break;
			default:
				fprintf(stderr, "invalid funct7 0x%02x at PC=0x%08x\n", funct7, PC);
				return -1;
			}
			break; 
		default:
			fprintf(stderr, "invalid funct3 %u at PC=0x%08x\n", funct3, PC);
			return -1;
		}
		break;
	case OP_LUI: 
		R[rd] = inst & 0xfffff000; 
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





