#include "sdb.h"
#include "watchpoint.h"
#include "include/npc.h"
#include "include/reg.h"
#include <stdio.h>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <readline/readline.h>
#include <readline/history.h>

static bool is_batch_mode = false;

void init_regex();

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
    display_wp();
  }else {
    printf("Unknown info subcommand: %s\n", arg);
  }
  return 0;
}

static int cmd_p(char *args){
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }
  bool success;
  uint32_t result = expr(args, &success);
  if (success) {
    printf("%s = 0x%08" PRIx32 "\n", args, result);
  } else {
    printf("Invalid expression: %s\n", args);
  }
  return 0;
}

static int cmd_w(char *args){
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }
  WP *wp = new_wp(args);
  if (wp == NULL) {
    return 0;
  }
  printf("Watchpoint %d set for expression: %s\n", wp->NO, wp->e);
  return 0;
}

static int cmd_d(char *args){
  char *no_str = strtok(NULL, " ");
  char *extra_arg = strtok(NULL, " ");
  long no;
  char trailing;
  if (args == NULL || no_str == NULL || extra_arg != NULL ||
      sscanf(no_str, "%ld %c", &no, &trailing) != 1 || no < 0) {
    printf("Usage: d N\n");
    return 0;
  }
  if (delete_wp(no)) {
    printf("Watchpoint %ld deleted.\n", no);
  } else {
    printf("No watchpoint number %ld.\n", no);
  }
  return 0;
}

static const struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table[] = {
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NPC", cmd_q },
  { "si"," Step through N instructions (default: 1)", cmd_si},
  { "info","Print program status", cmd_info},
  // { "x","Scan memory",cmd_x},
  { "p","Print value of expression", cmd_p},
  { "w","Set watchpoint", cmd_w },
  { "d","Delete watchpoint", cmd_d }
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
    cmd_c(NULL);
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

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
