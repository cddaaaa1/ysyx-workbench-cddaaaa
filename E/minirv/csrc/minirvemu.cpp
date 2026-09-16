#include "minirvemu.h"

#include <stdio.h>
#include <string.h>

#define OP_IMM      0x13 // 0010011, 立即数运算组 (OP-IMM)
#define OP_JALR     0x67 // 1100111, 跳转并链接组 (JALR)
#define OP_R        0x33 // 0110011, 寄存器-寄存器运算组 (OP)
#define OP_LUI      0x37 // 0110111, 大立即数组 (LUI, U 型)
#define OP_LOAD 	0x03 // 0000011, LOAD
#define OP_STORE 	0x23 // 0100011, STORE
#define OP_SYSTEM	0x73 // 1110011, SYSTEM

#define FUNCT3_ADDI 0x00 // OP_IMM  组内的 addi
#define FUNCT3_JALR 0x00 // OP_JALR 组内的 jalr
#define FUNCT3_ADD  0x00 // OP_R    组内的 add
#define FUNCT7_ADD  0x00 // OP_R    组内的 add (sub 的 funct7 为 0x20)
#define FUNCT3_LW	0x02 // OP_LOAD  组内的 lw
#define FUNCT3_LBU	0x04 // OP_LOAD  组内的 lbu
#define FUNCT3_SW	0x02 // OP_STORE 组内的 sw
#define FUNCT3_SB	0x00 // OP_STORE 组内的 sb
#define FUNCT3_PRIV	0x00 // OP_SYSTEM 组内的 ecall / ebreak

#define IMM_ECALL	0x00
#define IMM_EBREAK	0x01

static inline int32_t imm_i(uint32_t inst) // I 型: inst[31:20], 12 位有符号数
{
	return (int32_t)inst >> 20; 
}

static inline int32_t imm_s(uint32_t inst) // S 型: inst[31:25] | inst[11:7], 12 位有符号数
{
	uint32_t imm = ((inst >> 25) << 5) | ((inst >> 7) & 0x1f); // 两段拼成 12 位
	return (int32_t)(imm << 20) >> 20;                         // 符号扩展到 32 位
}

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

// 执行一条指令。
// 返回 0:  正常执行, 调用者应继续执行下一条指令
// 返回 1:  执行了 ebreak, 即程序自行声明结束 
// 返回 -1: 非法指令或访问越界, 应当中止
int ref_inst_cycle(void)
{
	uint32_t addr = PC >> 2; 
	if (addr >= REF_MEMORY_SIZE) { // PC 出 M 的范围
		fprintf(stderr, "PC 0x%08x is out of memory range\n", PC);
		return -1;
	}
	// 下面这些字段的位位置在 6 种格式里固定, 因此可以统一取出;
	// 立即数是唯一随格式变化的字段, 改用 imm_*() 按格式分别计算
	uint32_t inst   = M[addr];
	uint32_t opcode = inst & 0x7f;         
	uint32_t rd     = (inst >> 7) & 0x1f;  
	uint32_t funct3 = (inst >> 12) & 0x07; 
	uint32_t rs1    = (inst >> 15) & 0x1f; 
	uint32_t rs2    = (inst >> 20) & 0x1f;
	uint32_t funct7 = (inst >> 25) & 0x7f;
	
	uint32_t next_pc = PC + 4;

	switch (opcode) {
	case OP_IMM: 
		switch (funct3) {
		case FUNCT3_ADDI:
			R[rd] = R[rs1] + (uint32_t)imm_i(inst);
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
			uint32_t target = (R[rs1] + (uint32_t)imm_i(inst)) & ~1u; // 目标最低位清零
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
	case OP_LOAD:
		switch (funct3) {
		case FUNCT3_LW: {
			uint32_t vaddr = R[rs1] + (uint32_t)imm_i(inst);
			if ((vaddr >> 2) >= REF_MEMORY_SIZE) {
				fprintf(stderr, "lw: address 0x%08x out of memory range\n", vaddr);
				return -1;
			}
			R[rd] = M[vaddr >> 2];
			break;
		}
		case FUNCT3_LBU: {
			uint32_t vaddr = R[rs1] + (uint32_t)imm_i(inst);
			if ((vaddr >> 2) >= REF_MEMORY_SIZE) {
				fprintf(stderr, "lbu: address 0x%08x out of memory range\n", vaddr);
				return -1;
			}
			// 取出字内第 (vaddr & 0x3) 个字节, 零扩展后写回
			R[rd] = (M[vaddr >> 2] >> ((vaddr & 0x3) * 8)) & 0xff;
			break;
		}
		default:
			fprintf(stderr, "invalid funct3 %u at PC=0x%08x\n", funct3, PC);
			return -1;
		}
		break;
	case OP_STORE:
		switch (funct3) {
		case FUNCT3_SW: {
			uint32_t vaddr = R[rs1] + (uint32_t)imm_s(inst); 
			if ((vaddr >> 2) >= REF_MEMORY_SIZE) {
				fprintf(stderr, "sw: address 0x%08x out of memory range\n", vaddr);
				return -1;
			}
			M[vaddr >> 2] = R[rs2]; 
			break;
		}
		case FUNCT3_SB: {
			uint32_t vaddr = R[rs1] + (uint32_t)imm_s(inst);
			if ((vaddr >> 2) >= REF_MEMORY_SIZE) {
				fprintf(stderr, "sb: address 0x%08x out of memory range\n", vaddr);
				return -1;
			}
			uint32_t shift = (vaddr & 0x3) * 8; // 目标字节在字内的位偏移
			M[vaddr >> 2] = (M[vaddr >> 2] & ~(0xffu << shift))
			              | ((R[rs2] & 0xff) << shift);
			break;
		}
		default:
			fprintf(stderr, "invalid funct3 %u at PC=0x%08x\n", funct3, PC);
			return -1;
		}
		break;
	case OP_SYSTEM: // 系统指令组
		switch (funct3) {
		case FUNCT3_PRIV:
			if (imm_i(inst) == IMM_EBREAK) // 不能只认 opcode: ecall 与 ebreak 只差这一位
				return 1;                
			fprintf(stderr, "unsupported trap instruction at PC=0x%08x\n", PC);
			return -1;
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

