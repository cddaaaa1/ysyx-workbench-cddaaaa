#include <cstdlib>
#include "include/npc.h"
#include "monitor/monitor.h"
#include "monitor/sdb/sdb.h"

int main(int argc, char **argv) {
  if (!init_monitor(argc, argv)) {
    sim_cleanup();
    return EXIT_FAILURE;
  }
  sdb_mainloop();
  npc_statistics();
  const bool success = (npc_state.state == NPC_END && npc_state.halt_ret == 0)
                   || (!sdb_is_batch_mode() && npc_state.state == NPC_QUIT);
  sim_cleanup();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
