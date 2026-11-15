#ifndef NPC_TRACE_H
#define NPC_TRACE_H

#include <stdint.h>

#ifdef ITRACE
void init_trace(const char *log_file);
void itrace(uint32_t pc, uint32_t inst, bool print_step);
void iringbuf_dump();
#endif

#endif
