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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#define MAX_TOKEN_LEN 4096

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_HEXNUM,TK_DECNUM
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus 
  {"\\*", '*'},         // multiplication 
  {"\\/", '/'},         // divide
  {"==", TK_EQ},        // equal
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // right parentheses
  {"0[xX][0-9a-fA-F]+", TK_HEXNUM},  // hex number
  {"[0-9]+", TK_DECNUM},          // decimal number
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[MAX_TOKEN_LEN] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if (rules[i].token_type == TK_NOTYPE){
          break;
        }

        Assert(nr_token < ARRLEN(tokens), "%s", "too many tokens in expression\n");
        Assert(substr_len < sizeof(tokens[nr_token].str), "%s", "substring is too long\n");

        switch (rules[i].token_type) {
          case TK_HEXNUM:
          case TK_EQ:
          case TK_DECNUM:
          case '+': 
          case '-': 
          case '*':
          case '/':
          case '(': 
          case ')':
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          default:
            return false;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static int find_main_operator(int p, int q) {
  int count = 0;
  int main_op_idx = -1;
  int main_precedence = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      count++;
      continue;
    }
    if (tokens[i].type == ')') {
      count--;
      continue;
    }
    if (count != 0) {
      continue;
    }
    int precedence;
    switch (tokens[i].type) {
      case TK_EQ: precedence = 0; break;
      case '+':
      case '-': precedence = 1; break;
      case '*':
      case '/': precedence = 2; break;
      default: continue;
    }

    if (main_op_idx == -1 || precedence <= main_precedence) {
      main_op_idx = i;
      main_precedence = precedence;
    }
  }

  return main_op_idx;
}

static bool check_parentheses(int p, int q){
  if (tokens[p].type != '(' || tokens[q].type != ')'){
    return false;
  }
  int count = 0;
  for (int i = p + 1; i < q; i++){
    if (tokens[i].type == '('){
      count++;
    }else if (tokens[i].type == ')'){
      if (count == 0){ //check eg. 1  )  +  (  2
        return false;
      }
      count--;
    }
  }

  return count == 0;
}

static word_t eval(int p, int q) {
  if (p > q) {
    Log("Bad expression: p > q");
    return 0; 
  }
  else if (p == q) {
    word_t val; 
    switch (tokens[p].type){
      case TK_HEXNUM: 
        if (sscanf(tokens[p].str, "%x", &val) == 1) break;
      case TK_DECNUM: 
        if (sscanf(tokens[p].str, "%u", &val) == 1) break;
      default: break; 
    }
    return val;
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1);
  }
  else {
    int op = find_main_operator(p, q);
    word_t val1 = eval(p, op - 1);
    word_t val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      default: return 0;
    }
  }
  return 0;
}


word_t expr(char *e, bool *success) {
  *success = false;
  if (!make_token(e)) {
    return 0;
  }
  *success = true;
  word_t result = eval(0, nr_token - 1);
  return result;
}