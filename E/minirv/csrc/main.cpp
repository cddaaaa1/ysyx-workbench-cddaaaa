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


// addi 编码: imm[31:20] | rs1[19:15] | 000 | rd[11:7] | 0010011
// ABI 名对应: zero = x0, a0 = x10, a1 = x11

static const uint32_t prog_a0_20[]    = { 0x01400513 };             // addi a0,zero,20
static const uint32_t prog_a0_neg2[]    = { 0xffe00513 };             // addi a0,zero,-2

static const test_case_t test_cases[] = {
	// 用例名, 指令序列, 条数, 期望 PC, 期望的 GPR
	{ "addi a0,zero,20",
	  prog_a0_20, ARRAY_LEN(prog_a0_20), 4,
	  { REG(10, 20), REG_END } },
    
    { "addi a0,zero,-2",
	  prog_a0_neg2, ARRAY_LEN(prog_a0_neg2), 4,
	  { REG(10, 0xfffffffeu), REG_END } },
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
		if (ref_get_pc() >= (uint32_t)tc->inst_count * 4)
			break;
		if (ref_inst_cycle() < 0) {
			printf("         [FAIL] 执行第 %d 条指令时出错, PC=0x%08x\n",
			       cycle, ref_get_pc());
			return 1;
		}
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

