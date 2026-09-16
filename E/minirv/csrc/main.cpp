#include "minirvemu.h"

#include <stdio.h>
#include <stdlib.h>

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define MAX_CYCLES   1000        // 防止用例写错导致死循环
#define MAX_EXPECT   8           // 每个用例最多检查多少个 GPR
#define REG_NONE     0xffffffffu // 期望值列表的结束标记

typedef struct {
	uint32_t reg;
	uint32_t value;
} reg_expect_t;

typedef struct {
	const char *name;                     // 用例名
	const uint32_t *insts;                // 指令序列
	int inst_count;                       // 指令条数
	uint32_t expect_pc;                   // 执行完后期望的 PC
	reg_expect_t expect_regs[MAX_EXPECT]; // 期望的 GPR, 以 REG_END 结束
} test_case_t;

#define REG(r, v) { (r), (v) }
#define REG_END   { REG_NONE, 0u }

static const uint32_t prog_a0_20[]   = { 0x01400513 };  // addi a0,zero,20
static const uint32_t prog_a0_neg2[] = { 0xffe00513 };  // addi a0,zero,-2

static const uint32_t prog_jalr_doc[] = {
	0x01400513, //  0: addi a0,zero,20
	0x010000e7, //  4: jalr ra,16(zero)    -> ra = 8, 跳到 16
	0x00c000e7, //  8: jalr ra,12(zero)    -> ra = 12, 跳到 0xc
	0x00c00067, //  12: jalr zero,12(zero)  -> 跳回自己, 死循环 (halt)
	0x00a50513, // 16: addi a0,a0,10       (fun 的入口)
	0x00008067, // 20: jalr zero,0(ra)     -> 返回到 ra 指向的地址
};

static const uint32_t prog_jalr_rd_rs1[] = {
	0x01400093, // 0: addi ra,zero,20      -> ra = 20
	0x000080e7, // 4: jalr ra,0(ra)        -> 目标 = 20, 链接地址 = 8
};

static const uint32_t prog_add[] = {
	0x01400513, // 0: addi a0,zero,20      -> a0 = 20
	0x00100593, // 4: addi a1,zero,1       -> a1 = 1
	0x00b50533, // 8: add  a0,a0,a1        -> a0 = 21
};

static const uint32_t prog_lui[] = {
	0x123450b7, // 0: lui ra,0x12345        -> ra = 0x12345 << 12 = 0x12345000
};

static const uint32_t prog_lw[] = {
	0x00c00593, //  0: addi a1,zero,12     -> a1 = 12 (数据的字节地址)
	0x0005a503, //  4: lw   a0,0(a1)        -> a0 = M[12 >> 2] = M[3]
	0x00800067, //  8: jalr zero,8(zero)    -> 跳回自己, halt
	0x12345678, // 12: 数据 (M[3])
};

static const uint32_t prog_sw[] = {
	0x02a00113, //  0: addi x2,zero,42      -> x2 = 42 (待写入的数据)
	0x00400193, //  4: addi x3,zero,4       -> x3 = 4  (基址)
	0x0621a223, //  8: sw   x2,100(x3)      -> M[(4 + 100) >> 2] = M[26] = 42
	0x00000113, // 12: addi x2,zero,0      -> x2 = 0, 证明下面读的确实是内存
	0x0641a203, // 16: lw   x4,100(x3)     -> x4 = M[26] = 42
	0x01400067, // 20: jalr zero,20(zero)  -> 跳回自己, halt
};

static const test_case_t test_cases[] = {
	{ "addi a0,zero,20",
	  prog_a0_20, ARRAY_LEN(prog_a0_20), 4,
	  { REG(10, 20), REG_END } },

	{ "addi a0,zero,-2",
	  prog_a0_neg2, ARRAY_LEN(prog_a0_neg2), 4,
	  { REG(10, 0xfffffffeu), REG_END } },

	{ "jalr: 调用 fun、返回, 最后 halt 死循环 (文档测试程序)",
	  prog_jalr_doc, ARRAY_LEN(prog_jalr_doc), 0xc,
	  { REG(1, 12), REG(10, 30), REG_END } },

	{ "jalr 的 rd 与 rs1 是同一个寄存器",
	  prog_jalr_rd_rs1, ARRAY_LEN(prog_jalr_rd_rs1), 0x14,
	  { REG(1, 8), REG_END } },

	{ "add: a0 = 20 + 1",
	  prog_add, ARRAY_LEN(prog_add), 12,
	  { REG(10, 21), REG(11, 1), REG_END } },

	{ "LUI ra, 0x12345",
	  prog_lui, ARRAY_LEN(prog_lui), 4,
	  { REG(1, 0x12345000), REG_END } },

	{ "lw: 从 M[3] 读出数据 0x12345678",
	  prog_lw, ARRAY_LEN(prog_lw), 8,
	  { REG(10, 0x12345678), REG(11, 12), REG_END } },

	{ "sw: sw x2,100(x3) 后再 lw 读回",
	  prog_sw, ARRAY_LEN(prog_sw), 0x14,
	  { REG(2, 0), REG(3, 4), REG(4, 42), REG_END } },
};

// 打印现场, 便于定位失败原因
static void dump_regs(void)
{
	uint32_t *R = ref_get_regs();

	for (int i = 0; i < REF_REGISTER_COUNT; i++)
		printf("           x%-2d = 0x%08x (%d)\n", i, R[i], (int32_t)R[i]);
}

// 执行一个用例, 返回不符项的个数 (0 表示通过)
static int run_case(const test_case_t *tc)
{
	uint32_t *R;
	int failures = 0;

	if (ref_load_image(tc->insts, tc->inst_count) < 0) {
		printf("         [FAIL] 程序装入失败\n");
		return 1;
	}
	ref_reset();

	for (int cycle = 0; cycle < MAX_CYCLES; cycle++) {
		uint32_t pc_before = ref_get_pc();

		if (pc_before >= (uint32_t)tc->inst_count * 4)
			break;
		if (ref_inst_cycle() < 0) {
			printf("         [FAIL] 执行第 %d 条指令时出错, PC=0x%08x\n",
			       cycle, ref_get_pc());
			return 1;
		}
		if (ref_get_pc() == pc_before) // 跳回自身, 视为程序结束
			break;
	}

	R = ref_get_regs();

	if (ref_get_pc() != tc->expect_pc) {
		printf("         [FAIL] PC: 期望 0x%08x, 实际 0x%08x\n",
		       tc->expect_pc, ref_get_pc());
		failures++;
	}

	for (int i = 0; i < MAX_EXPECT && tc->expect_regs[i].reg != REG_NONE; i++) {
		uint32_t reg = tc->expect_regs[i].reg;
		uint32_t expect = tc->expect_regs[i].value;

		if (R[reg] != expect) {
			printf("         [FAIL] x%u: 期望 0x%08x (%d), 实际 0x%08x (%d)\n",
			       reg, expect, (int32_t)expect, R[reg], (int32_t)R[reg]);
			failures++;
		}
	}

	if (R[0] != 0) { // x0 恒为 0, 与用例无关
		printf("         [FAIL] x0: 期望 0, 实际 0x%08x\n", R[0]);
		failures++;
	}

	if (failures > 0)
		dump_regs();

	return failures;
}

int main(void)
{
	int total = ARRAY_LEN(test_cases);
	int passed = 0;

	printf("=== minirvEMU 测试: 共 %d 个用例 ===\n\n", total);

	for (int i = 0; i < total; i++) {
		const test_case_t *tc = &test_cases[i];
		int failures = run_case(tc);

		if (failures == 0) {
			passed++;
			printf("[ PASS ] %s\n\n", tc->name);
		} else {
			printf("[ FAIL ] %s (%d 项不符)\n\n", tc->name, failures);
		}
	}

	printf("================================\n");
	printf("通过 %d / %d\n", passed, total);

	return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}

