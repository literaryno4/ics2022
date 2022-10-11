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

#include "sdb.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* wp_head() {
  return head;
}

WP* new_wp() {
  assert(free_ != NULL);
  WP* ans = free_;
  ans->next = head;
  head = ans;

  free_ = free_->next;
  return ans;
}

void print_watchpoints() {
  WP* tmp = head;
  while (tmp != NULL) {
    printf("Watchpoint number: %d, val: %d, expr: %s\n", tmp->NO, tmp->old_val, tmp->expr);
    tmp = tmp->next;
  }
}

void free_wp(int no) {
  if (head != NULL && head->NO == no) {
    head->next = free_;
    free_ = head;
    head = head->next;
    printf("deleted watchpoint %d\n", no);
    return;
  }
  WP* tmp = head;
  while (tmp != NULL && tmp->next != NULL) {
    if (tmp->next->NO == no) {
      break;
    }
    tmp = tmp->next;
  }
  if (tmp == NULL || tmp->next == NULL) {
    printf("watchpoint not exists, watchpoint list: \n");
    print_watchpoints();
  } else {
    WP* t = tmp->next->next;
    tmp->next->next = free_;
    free_ = tmp->next;
    tmp->next = t;
    printf("deleted watchpoint %d\n", no);
  }
}
