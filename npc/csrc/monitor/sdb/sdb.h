#ifndef NPC_SDB_H
#define NPC_SDB_H
#include <stdint.h>

uint32_t expr(char *e, bool *success);
void init_sdb();
void sdb_set_batch_mode();
bool sdb_is_batch_mode();
void sdb_mainloop();
#endif
