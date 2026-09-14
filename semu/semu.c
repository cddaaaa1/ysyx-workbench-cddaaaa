#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MEMORY_SIZE 256
#define REGISTER_COUNT 4
#define PROGRAM_SIZE 7

#define OP_ADD   0x00
#define OP_LI    0x80
#define OP_BNER0 0xc0

static uint8_t PC;
static uint8_t R[REGISTER_COUNT];
static uint8_t M[MEMORY_SIZE];

static void load_program(const char *path)
{
	FILE *program = fopen(path, "r");
	unsigned value;
	int address = 0;

	if (program == NULL) { 
		perror(path);
		exit(EXIT_FAILURE);
	}

	while (address < MEMORY_SIZE && fscanf(program, "%x", &value) == 1) //逐条读取指令
		M[address++] = (uint8_t)value;

	fclose(program); 
	if (address != PROGRAM_SIZE) { //检查程序长度
		fprintf(stderr, "expected %d instructions, loaded %d\n",
				PROGRAM_SIZE, address);
		exit(EXIT_FAILURE);
	}
}

static void inst_cycle(void)
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
		assert(!"invalid sISA opcode");
	}

	PC = next_pc;
}

int main(int argc, char **argv)
{
	const char *program_path = argc > 1 ? argv[1] : "scpu/program/sum.hex";
	load_program(program_path);

	for (int cycle = 0; cycle < 64 && PC != PROGRAM_SIZE; cycle++) {
		printf("cycle=%d pc=%u r0=%u r1=%u r2=%u inst=0x%02x\n",
			   cycle, PC, R[0], R[1], R[2], M[PC]);
		inst_cycle();
	}

	assert(PC == PROGRAM_SIZE);
	assert(R[0] == 11);
	assert(R[1] == 11);
	assert(R[2] == 55);
	assert(R[3] == 1);
	printf("sEMU PASS: R2 = 1 + 2 + ... + 10 = %u\n", R[2]);
	return 0;
}


