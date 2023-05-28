#ifndef __ITRACE_H__
#define __ITRACE_H__

#include <common.h>

void trace_inst(word_t pc, uint32_t inst);
void display_inst();
void display_pread(paddr_t addr, int len);
void display_pwrite(paddr_t addr, int len, word_t data);

#endif
