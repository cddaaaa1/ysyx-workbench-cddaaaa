/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

#define TEST_EXPR 1

#if TEST_EXPR
#include "monitor/sdb/sdb.h"
static int test_expr_file(const char *path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    printf("Cannot open %s\n", path);
    return -1;
  }

  char *line = NULL;
  size_t cap = 0;
  int total = 0;
  int failed = 0;

  while (getline(&line, &cap, fp) >= 0) {
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0') {
      continue;
    }

    unsigned expected;
    int offset = 0;
    if (sscanf(line, "%u %n", &expected, &offset) != 1) {
      printf("Invalid test case: %s\n", line);
      failed++;
      continue;
    }

    bool success;
    word_t actual = expr(line + offset, &success);
    total++;

    if (!success || actual != (word_t)expected) {
      printf("FAIL: %s\n", line);
      printf("  expected = " FMT_WORD ", actual = " FMT_WORD
             ", success = %s\n",
             (word_t)expected, actual, success ? "true" : "false");
      failed++;
    }
  }

  free(line);
  fclose(fp);

  printf("Expr tests: %d passed, %d failed, %d total\n",
         total - failed, failed, total);
  return failed;
}

int main(int argc, char *argv[]) {
  init_monitor(argc, argv); 
  int failed = test_expr_file("/home/cddaaaa/Projects/ysyx/ysyx-workbench-cddaaaa/nemu/tools/gen-expr/input");
  return failed == 0 ? 0 : 1;
}
#else

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
#endif 