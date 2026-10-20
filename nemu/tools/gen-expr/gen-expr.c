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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>


// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int choose(int n) {
  return rand() % n;
}


static void gen_num() {
  sprintf(buf + strlen(buf), "%uu", (unsigned int)choose(100));
}

static void gen(char c) {
  sprintf(buf + strlen(buf), "%c", c);
}

static void gen_space(void) {
  switch (choose(3)) {
    case 0:
      break;
    case 1:
      gen(' ');
      break;
    default :
      gen(' ');
      gen(' ');
      break;
  }
}

static void gen_rand_op() {
  switch (choose(4)) {
    case 0: gen('+'); break;
    case 1: gen('-'); break;
    case 2: gen('*'); break;
    default: gen('/'); break;
  }
}

void gen_rand_expr(int depth) {
  if (depth > 10) {
    gen_num();
    return;
  }
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_space();gen_rand_expr(depth + 1); gen_space(); gen(')'); break; 

    default: gen_rand_expr(depth + 1); gen_space(); gen_rand_op(); gen_space(); gen_rand_expr(depth + 1); break;
  }
}

void remove_u_sign(char *p) {
  char *q = p;
  while ((q = strchr(q, 'u')) != NULL) {
    strcpy(code_buf, q + 1);
    strcpy(q, code_buf);
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0] = '\0'; 
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    int read_ret = fscanf(fp, "%u", &result);
    int close_ret = pclose(fp);

    if (read_ret != 1 || close_ret != 0) {
      continue;
    }

    remove_u_sign(buf);
    printf("%u %s\n", result, buf);
  }
  return 0;
}
