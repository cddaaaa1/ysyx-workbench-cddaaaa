#ifndef NPC_H
#define NPC_H

#include <stdint.h>
#ifdef YSYXSOC
#include <VSimTop.h>
using TOP = VSimTop;
#else
#include <Vnpc_top.h>
using TOP = Vnpc_top;
#endif
#ifdef NVBOARD
#include <nvboard.h>
#endif

#include "macro.h"
#include "pmem.h"

struct SimState {
	unsigned long long cycle = 0;      // 当前周期数, RTC 用
	bool               retired = false; // 本拍提交了指令
	uint32_t           retire_pc = 0;
};

extern SimState sim;

#endif
