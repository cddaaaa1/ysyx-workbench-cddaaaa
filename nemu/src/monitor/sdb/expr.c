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

#define MAX_TOKEN 4096

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_HEXNUM,TK_DECNUM
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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if (rules[i].token_type == TK_NOTYPE){
          break;
        }

        if (nr_token >= ARRLEN(tokens)) {
          Log("Too many tokens in expression, number of tokens: %d", nr_token);
          return false;
        }
         
        if (substr_len >= sizeof(tokens[nr_token].str)) {
          Log("Substring is too long, substring length: %d", substr_len);
          return false;
        }

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
            Log("Unknown token type: %d", rules[i].token_type);
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

static bool eval(int p, int q, word_t *result) {
  ParenStatus paren_status = check_parentheses(p, q);
  if (p > q) {
    Log("Bad expression: p > q");
    return false;
  }
  else if (p == q) {
    switch (tokens[p].type){
      case TK_HEXNUM: 
        return sscanf(tokens[p].str, "%x", result) == 1;
      case TK_DECNUM: 
        return sscanf(tokens[p].str, "%u", result) == 1;
      default: 
        Log("Bad expression: unexpected token type %d", tokens[p].type);
        return false; 
    }
  }
  else if (paren_status == PAREN_INVALID) {
    Log("Bad expression: parentheses are not matched");
    return false;
  }
  else if (paren_status == PAREN_ENCLOSED) {
    return eval(p + 1, q - 1,result);
  }
  else {
    int op = find_main_operator(p, q);
    if (op == -1) {
      Log("Bad expression: no main operator found,");
      return false;
    }
    word_t val1;
    word_t val2;

    if (!eval(p, op - 1, &val1)) {
      return false;
    }

    if (!eval(op + 1, q, &val2)) {
      return false;
    }

    switch (tokens[op].type) {
      case '+': *result = val1 + val2; return true;
      case '-': *result = val1 - val2; return true;
      case '*': *result = val1 * val2; return true;
      case '/':
        if (val2 == 0) {
          Log("Division by zero error");
          return false;
        }else {
          *result = val1 / val2;
          return true;
        }
      case TK_EQ: *result = val1 == val2; return true;
      default: return false;
    }
  }
}

word_t expr(char *e, bool *success) {
  *success = false;
  if (!make_token(e)) {
    return 0;
  }

  word_t result;
  if (eval(0, nr_token - 1, &result)) {
    *success = true;
  } else {
    *success = false;
    return 0;
  }
  return result;
}