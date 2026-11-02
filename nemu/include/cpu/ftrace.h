#ifndef __FTRACE_H__
#define __FTRACE_H__
#include <common.h>

#ifdef CONFIG_FTRACE
bool init_ftrace(const char *ftrace_file);
const char *ftrace_find_function_name(uint64_t addr);
void ftrace_call(uint64_t pc, uint64_t target);
void ftrace_ret(uint64_t pc, uint64_t target);
#endif
#endif
