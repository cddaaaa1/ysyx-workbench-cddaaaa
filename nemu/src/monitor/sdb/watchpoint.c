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
#include "watchpoint.h"

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

WP *new_wp(char *e) {
  Assert(free_ != NULL, "No free watchpoint available.");
  bool success;
  word_t expr_val = expr(e, &success);
  if (!success) {
    Log("Invalid expression for watchpoint: %s\n", e);
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;
  wp->next = NULL;

  wp->val = expr_val;
  wp->e = strdup(e);

  wp->next = head;
  head = wp;
  return wp;
}

void free_wp(WP *wp){
  Assert(wp != NULL, "Invalid watchpoint.");
  Assert(head != NULL, "No active watchpoint.");

  if (head == wp) {
    head = wp->next;
  }else {
    WP *prev = head;
    while (prev->next != NULL && prev->next != wp) {
      prev = prev->next;
    }
    Assert(prev->next == wp, "Watchpoint not found in the list.");
    prev->next = wp->next;
  }

  free(wp->e);
  wp->e = NULL;
  wp->val = 0;
  wp->next = free_;
  free_ = wp;
}

bool check_wp() {
  WP *current = head;
  bool nemu_stop = false;
  while (current != NULL) {
    bool success;
    word_t new_val = expr(current->e, &success);
    if (!success) {
      printf("Watchpoint %d evaluation failed: %s\n", current->NO, current->e);
      nemu_stop = true;
      break;
    }
    if (new_val != current->val) {
      printf("Watchpoint %d: %s\n", current->NO, current->e);
      printf("Old value: " FMT_WORD "\n New value: " FMT_WORD "\n", current->val, new_val);
      current->val = new_val;
      nemu_stop = true;
    }
    current = current->next;
  }
  return nemu_stop;
}

void display_wp(void) {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num  Expression                             Value\n");
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    printf("%-4d %-36s " FMT_WORD "\n", wp->NO, wp->e, wp->val);
  }
}

bool delete_wp(long no) {
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    if (wp->NO == no) {
      free_wp(wp);
      return true;
    }
  }
  return false;
}
