#include "sdb.h"
#include "include/npc.h"
#include "include/reg.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <readline/readline.h>
#include <readline/history.h>

static bool is_batch_mode = false;

static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(npc) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}
static int cmd_c(char *args) {
  npc_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  npc_state.state = NPC_QUIT;
  return -1;
}

static int cmd_si(char *args){
  char* arg = strtok(NULL, " ");
  int steps;
  if (arg == NULL) {
    /* no argument, default step one instruction */
    npc_exec(1);
  }else if (sscanf(arg, "%d", &steps) == 1 && steps > 0)  {
    npc_exec(steps);
  }else {
    printf("Usage: si [positive integer]\n");
  }
  
  return 0;
}

static int cmd_info(char *args){
  char* arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("no argument: info r|w\n");
  }else if (strcmp(arg, "r") == 0) {
    reg_display();
  }else if (strcmp(arg, "w") == 0) {
    // display_wp();
    printf("display watch point not implemented\n");
  }else {
    printf("Unknown info subcommand: %s\n", arg);
  }
  return 0;
}

static const struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table[] = {
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si"," Step through N instructions (default: 1)", cmd_si},
  { "info","Print program status", cmd_info},
  // { "x","Scan memory",cmd_x},
  // { "p","Print value of expression", cmd_p},
  // { "w","Set watchpoint", cmd_w },
  // { "d","Delete watchpoint", cmd_d }
};


#define NR_CMD ARRLEN(cmd_table)

void sdb_set_batch_mode() { 
  is_batch_mode = true; 
}

bool sdb_is_batch_mode(){
  return is_batch_mode;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(nullptr);
    return;
  }
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
