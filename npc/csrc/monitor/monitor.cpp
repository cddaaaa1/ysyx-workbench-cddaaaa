#include "monitor.h"
#include "include/npc.h"
#include "sdb/sdb.h"
#include <stdio.h>
#include <getopt.h>
#include <cstdlib>

static const char *img_file = NULL;
static bool batch = false;

static int parse_args(int argc, char *argv[]) 
{
    const struct option table[] = {
        {"batch"    , no_argument      , NULL, 'b'},
        {"help"     , no_argument      , NULL, 'h'},
        {"log"      , required_argument, NULL, 'l'},
        {"diff"     , required_argument, NULL, 'd'},
        {"elf"      , required_argument, NULL, 'e'},
        {0          , 0                , NULL,  0 },
    };
    int o;
    while ( (o = getopt_long(argc, argv, "-bhl:d:e:", table, NULL)) != -1) {
        switch (o) {
            case 'b': sdb_set_batch_mode(); break;
            // case 'l': log_file     = optarg; break;
            // case 'd': diff_so_file = optarg; break;
            // case 'e': elf_file     = optarg; break; 
            case 1  : img_file     = optarg; return 0;
            default :
            printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
            printf("\t-b,--batch              run with batch mode\n");
            // printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
            // printf("\t-l,--log=FILE           output log to FILE\n");
            // printf("\t-e,--elf=FILE           input elf FILE to support MTRACE\n");
            printf("\n");
            exit(0);

          }
    }
    if (img_file == NULL) return -1;

    return 0;
}

bool monitor_is_batch_mode() { return batch; }

bool init_monitor(int argc, char *argv[]) {
  if (parse_args(argc, argv)) return false;

#ifdef YSYXSOC  
  flash_load(img_file);
#else
  if (pmem_load(img_file) < 0) return false;
#endif

  sim_init(argc, argv);
  sim_reset(100);
  if (batch) sdb_set_batch_mode();
  return true;
}