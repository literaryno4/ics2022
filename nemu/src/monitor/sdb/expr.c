/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include "memory/vaddr.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_INTEGER, TK_REGNAME, TK_NEQ, TK_LEQ, TK_GEQ, TK_LOGICAND, TK_LOGICOR, TK_DEREF

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"([\\+\\-]?[0-9]+)|([\\+\\-]?0[x|X][0-9a-fA-F]+)", TK_INTEGER}, 

  {"\\$((pc)|($0)|(ra)|(sp)|(gp)|(tp)|(t0)|(t1)|(t2)|(s0)|(s1)|(a0)|(a1)|(a2)|(a3)|(a4)|(a5)|(a6)|(a7)|(s2)|(s3)|(s4)|(s5)|(s6)|(s7)|(s8)|(s9)|(s10)|(s11)|(t3)|(t4)|(t5)|(t6))", TK_REGNAME},

  {" +", TK_NOTYPE},                // spaces
  {"\\+", '+'},                     // plus
  {"\\-", '-'},                     // sub
  {"\\*", '*'},                     // multiply
  {"\\/", '/'},                     // div
  {"\\(", '('},                     // div
  {"\\)", ')'},                     // div
  {"<=", TK_LEQ},
  {">=", TK_GEQ},
  {"==", TK_EQ},                    // equal
  {"!=", TK_NEQ},
  {"\\&\\&", TK_LOGICAND},
  {"\\|\\|", TK_LOGICOR},
};

enum priority {
  LOGIC = 1,
  LEQ_GEQ, 
  NEQ_EQ,
  PLUS_SUB,
  MULTI_DIV,
  DEREF,
  OTHER,
};

static int priority_of_op(int type) {
  switch (type) {
    case TK_LOGICAND: return LOGIC;
    case TK_LOGICOR: return LOGIC;
    case TK_LEQ:
    case TK_GEQ:
      return LEQ_GEQ;
    case TK_EQ:
    case TK_NEQ:
      return NEQ_EQ;
    case '+':
    case '-':
      return PLUS_SUB;
    case '*':
    case '/':
      return MULTI_DIV;
    case TK_DEREF:
      return DEREF;
    default: return OTHER;
  }
}

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

static Token tokens[128] __attribute__((used)) = {};
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

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //   i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case '*':
            // dereference type
            if (nr_token == 0 || 
              (tokens[nr_token -1].type != TK_INTEGER && 
              tokens[nr_token -1].type != ')' && 
              tokens[nr_token -1].type != TK_REGNAME)) {
              tokens[nr_token].type = TK_DEREF;
              ++nr_token;
              break;
            }
          case TK_INTEGER:
          case TK_REGNAME:
            if (nr_token > 0 && 
              (tokens[nr_token - 1].type == TK_INTEGER ||
                tokens[nr_token - 1].type == ')') && 
              priority_of_op(substr_start[0]) == PLUS_SUB) {
              tokens[nr_token].type = '+';
              ++nr_token;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
          default: 
            tokens[nr_token].type = rules[i].token_type;
            ++nr_token;
            break;
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

bool bad_expr = false;

bool check_parentheses(int p, int q) {
  if (tokens[p].type == '(' && tokens[q].type == ')') {
    int nc = 0;
    for (int i = p + 1; i < q; ++i) {
      if (tokens[i].type == '(') {
        ++nc;
      } else if (tokens[i].type == ')'){
        if (nc > 0) {
          --nc;
        } else {
          return false;
        }
      }
    }
    if (nc != 0) {
      Log("bad parentheses\n");
      bad_expr = true;
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

static int primary_operator(int p, int q) {
  int ans = p, prior = OTHER;
  int open = 0;
  for (; p <= q; ++p) {
    int type = tokens[p].type;
    int cur_prior = priority_of_op(type);

    // by pass expr in parentheses
    if (type == '(') {
      ++open;
    } else if (type == ')') {
      if (open == 0) {
        Log("parentheses not match\n");
        bad_expr = true;
      } else {
        --open;
      }
    // update
    } else if (open == 0 && cur_prior <= prior) {
      ans = p;
      prior = cur_prior;
    }
  }
  return ans;
}

static word_t str2intger(char* str) {
  int base;
  if (str[0] == '$') {
    bool success;
    return isa_reg_str2val(str + 1, &success);
  }
  if (strlen(str) < 2) {
    base = 10;
  } else {
    base = str[1] == 'x' ? 16 : 10;
  }
  return strtol(str, NULL, base);
}

static word_t eval(int p, int q) {
  if (bad_expr) {
    return 0;
  }

  if (p > q) {
    Log("expr position error\n");
    bad_expr = true;
  } else if (p == q) {
    return str2intger(tokens[p].str);
  } else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1);
  } else {
    int op = primary_operator(p, q);
    if (tokens[op].type == DEREF) {
      // use vaddr_read to dereference pointer
      return vaddr_read(eval(op + 1, q), 4);
    }
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case TK_LOGICAND: return val1 && val2;
      case TK_LOGICOR: return val1 || val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_LEQ: return val1 <= val2;
      case TK_GEQ: return val1 >= val2;
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': {
        if (val2 == 0) {
          Log(ANSI_FMT("warning: divide 0", ANSI_FG_RED));
          return 0;
        }
        return val1 / val2;
      }
      default: 
        bad_expr = true;
        return 0;
    }
  }
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */

  bad_expr = false;
  word_t ans = eval(0, nr_token - 1);
  nr_token = 0;
  if (bad_expr) {
    *success = false;
    return 0;
  }
  *success = true;
  return ans;
}
