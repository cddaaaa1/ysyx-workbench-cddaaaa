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

#include "sdb.h"
#include "include/macro.h"
#include "include/reg.h"
#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>

/* NPC uses uint32_t for RV32 values and stdio instead of NEMU logging. */

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#define MAX_TOKEN 4096

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_HEXNUM,TK_DECNUM, TK_REGISTER, TK_NEQ , TK_AND
  /* TODO: Add more token types */

};

typedef enum {
  PAREN_INVALID,
  PAREN_NOT_ENCLOSED,
  PAREN_ENCLOSED
} ParenStatus;

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
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // logical and
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // right parentheses
  {"0[xX][0-9a-fA-F]+", TK_HEXNUM},  // hex number
  {"[0-9]+", TK_DECNUM},          // decimal number
  {"\\$[0-9a-z]+", TK_REGISTER}   // register
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
      fprintf(stderr, "regex compilation failed: %s\n%s\n", error_msg, rules[i].regex);
      abort();
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[MAX_TOKEN] __attribute__((used)) = {};
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

        // printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if (rules[i].token_type == TK_NOTYPE){
          break;
        }

        if (nr_token >= ARRLEN(tokens)) {
          printf("Too many tokens in expression, number of tokens: %d\n", nr_token);
          return false;
        }

        if (substr_len >= sizeof(tokens[nr_token].str)) {
          printf("Substring is too long, substring length: %d\n", substr_len);
          return false;
        }

        switch (rules[i].token_type) {
          case TK_HEXNUM:
          case TK_EQ:
          case TK_NEQ:
          case TK_DECNUM:
          case TK_REGISTER:
          case TK_AND:
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
            printf("Unknown token type: %d\n", rules[i].token_type);
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
      case TK_AND: precedence = 0; break;
      case TK_EQ:
      case TK_NEQ: precedence = 1; break;
      case '+':
      case '-': precedence = 2; break;
      case '*':
      case '/': precedence = 3; break;
      default: continue;
    }

    /* All supported operators are binary and left-associative. */
    if (main_op_idx == -1 || precedence < main_precedence || precedence == main_precedence) {
      main_op_idx = i;
      main_precedence = precedence;
    }
  }
  return main_op_idx;
}

static ParenStatus check_parentheses(int p, int q) {
  if (p > q) {
    return PAREN_INVALID;
  }

  int depth = 0;
  bool enclosed = tokens[p].type == '(' && tokens[q].type == ')';

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      depth++;
    } else if (tokens[i].type == ')') {
      if (depth == 0) {
        return PAREN_INVALID;
      }
      depth--;
    }

    if (depth == 0 && i < q) {
      enclosed = false;
    }
  }

  if (depth != 0) return PAREN_INVALID;

  return enclosed ? PAREN_ENCLOSED : PAREN_NOT_ENCLOSED;
}

static uint32_t eval(int p, int q, bool *success) {
  *success = false;
  ParenStatus paren_status = check_parentheses(p, q);
  if (p > q) {
    printf("Bad expression: p > q\n");
    return 0;
  }
  else if (p == q) {
    uint32_t value;
    switch (tokens[p].type){
      case TK_HEXNUM:
        *success = sscanf(tokens[p].str, "%" SCNx32, &value) == 1;
        return *success ? value : 0;
      case TK_DECNUM:
        *success = sscanf(tokens[p].str, "%" SCNu32, &value) == 1;
        return *success ? value : 0;
      case TK_REGISTER: {
        value = reg_str2val(tokens[p].str, success);
        if (!*success) {
          printf("Invalid register: %s\n", tokens[p].str);
          return 0;
        }
        return value;
      }
      default:
        printf("Bad expression: unexpected token type %d\n", tokens[p].type);
        return 0;
    }
  }
  else if (paren_status == PAREN_INVALID) {
    printf("Bad expression: parentheses are not matched\n");
    return 0;
  }
  else if (paren_status == PAREN_ENCLOSED) {
    return eval(p + 1, q - 1, success);
  }
  else {
    int op = find_main_operator(p, q);
    if (op == -1) {
      printf("Bad expression: no main operator found,\n");
      return 0;
    }
    uint32_t result = 0;
    uint32_t val1 = eval(p, op - 1, success);
    if (!*success) return 0;
    uint32_t val2 = eval(op + 1, q, success);
    if (!*success) return 0;

    *success = false;
    switch (tokens[op].type) {
      case '+': result = val1 + val2; break;
      case '-': result = val1 - val2; break;
      case '*': result = val1 * val2; break;
      case '/':
        if (val2 == 0) {
          printf("Division by zero error\n");
          *success = false;
          return 0;
        }
        result = val1 / val2;
        break;
      case TK_EQ: result = val1 == val2; break;
      case TK_NEQ: result = val1 != val2; break;
      case TK_AND: result = val1 && val2; break;
      default: *success = false; return 0;
    }
    *success = true;
    return result;
  }
}

uint32_t expr(char *e, bool *success) {
  *success = false;
  if (!make_token(e)) {
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i-1].type != TK_HEXNUM && tokens[i-1].type != TK_DECNUM && tokens[i-1].type != TK_REGISTER && tokens[i-1].type != ')')) ) {
      /* Keep binary multiplication, but do not access guest memory. */
      printf("Dereference is not supported.\n");
      return 0;
    }
  }

  return eval(0, nr_token - 1, success);
}
