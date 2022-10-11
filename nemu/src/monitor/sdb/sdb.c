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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "memory/vaddr.h"
#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static int cmd_info(char *args);

static int cmd_help(char *args);
static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Usage: si [N], step n instruction(s), N=1 by default", cmd_si},
  { "info", "Usage: info SUBCMD\n"
    "info r, print status of registers;\n"
    "info w, prinf status of watchpoints.", cmd_info},
  { "x", "Usage: x N EXPR, prinf N 4 bytes of addr of EXPR", cmd_x},
  { "p", "Usage: p EXPR, calculate of EXPR", cmd_p},
  { "w", "Usage: w EXPR, set watchpoint, stop when EXPR changes", cmd_w},
  { "d", "Usage: d N, delete watchpoint of number N", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int check_mem(vaddr_t addr) {
  if (!in_pmem(addr)) {
    Log("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
    return 1;
  }
  return 0;
}

static int cmd_x(char *args) {
  char* arg = strtok(NULL, " ");
  int i, n = atoi(arg);
  vaddr_t addr;
  arg = strtok(NULL, " ");
  addr = (vaddr_t)strtol(arg, NULL, 16);
  
  for (i = 1; i <= n; ++i) { 
    if (check_mem(addr)) {
      break;
    }
    if ((i - 1) % 4 == 0) {
      printf("%016x: ", addr);
    }
    printf("%08x ", vaddr_read(addr, 4));
    addr += 4;
    if (i % 4 == 0) {
      printf("\n");
    }
  }
  if (i % 4 != 0) {
    printf("\n");
  }
  return 0;
}

static int cmd_p(char *args) {
  char* arg = strtok(NULL, "\n");
  bool success;

  word_t ans = expr(arg, &success);
  if (success) {
    printf("%d 0x%x\n", ans, ans);
  } else {
    printf("bad expr, please check!\n");
  }

  return 0;
}

static int cmd_w(char *args) {
  char* arg = strtok(NULL, "\n");
  bool success;

  word_t ans = expr(arg, &success);
  if (success) {
    WP* wp = new_wp();
    wp->old_val = ans; 
    strcpy(wp->expr, arg);
  } else {
    printf("bad expr, please check!\n");
    return 0;
  }

  return 0;
}
static int cmd_d(char *args) {
  free_wp(atoi(strtok(NULL, "\n")));
  return 0;
}

static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int n;

  if (arg == NULL) {
    cpu_exec(1);  
  }
  else {
    n = atoi(arg);
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    printf("Usage: info SUBCMD\n");
  }
  else {
    if (strcmp(arg, "r") == 0) {
      isa_reg_display();
    } else if (strcmp(arg, "w") == 0) {
    } else {
      printf("Unknown command '%s'\n", arg);
    }
  }
  return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
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

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

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

void test_expr() {
  FILE* stream;
  char* line = NULL;
  size_t len = 0;
  ssize_t nread;
  word_t result;
  char* exp;
  bool success;

  if ((stream = fopen("/home/chao/ics2022/nemu/src/input", "r")) == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  while ((nread = getline(&line, &len, stream)) != -1) {
    result = atoi(strtok(line, " "));
    exp = strtok(NULL, "\n");
    printf("%d %s: ", result, exp);
    word_t ans = expr(exp, &success);
    if (result == ans) {
      printf( ANSI_FG_GREEN "pass" ANSI_NONE "\n");
    } else {
      printf( ANSI_FG_RED "failed, %d != %d" ANSI_NONE "\n", result, ans);
    }
  }

  free(line);
  fclose(stream);
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
  // test_expr();
}
