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
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* NPC uses uint32_t values and standard assertions and output. */

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
  /* Keep the debugger usable when the watchpoint pool is full. */
  if (free_ == NULL) {
    printf("No free watchpoint available.\n");
    return NULL;
  }
  bool success;
  uint32_t expr_val = expr(e, &success);
  if (!success) {
    printf("Invalid expression for watchpoint: %s\n", e);
    return NULL;
  }

  /* Do not consume a pool entry if copying the expression fails. */
  char *expr_copy = strdup(e);
  if (expr_copy == NULL) {
    printf("Failed to copy watchpoint expression.\n");
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;
  wp->next = NULL;

  wp->val = expr_val;
  wp->e = expr_copy;

  wp->next = head;
  head = wp;
  return wp;
}

void free_wp(WP *wp){
  assert(wp != NULL && "Invalid watchpoint.");
  assert(head != NULL && "No active watchpoint.");

  if (head == wp) {
    head = wp->next;
  }else {
    WP *prev = head;
    while (prev->next != NULL && prev->next != wp) {
      prev = prev->next;
    }
    assert(prev->next == wp && "Watchpoint not found in the list.");
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
  bool npc_stop = false;
  while (current != NULL) {
    bool success;
    uint32_t new_val = expr(current->e, &success);
    if (!success) {
      printf("Watchpoint %d evaluation failed: %s\n", current->NO, current->e);
      npc_stop = true;
      break;
    }
    if (new_val != current->val) {
      printf("Watchpoint %d: %s\n", current->NO, current->e);
      printf("Old value: 0x%08" PRIx32 "\n New value: 0x%08" PRIx32 "\n", current->val, new_val);
      current->val = new_val;
      npc_stop = true;
    }
    current = current->next;
  }
  return npc_stop;
}

void display_wp(void) {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num  Expression                             Value\n");
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    printf("%-4d %-36s 0x%08" PRIx32 "\n", wp->NO, wp->e, wp->val);
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
