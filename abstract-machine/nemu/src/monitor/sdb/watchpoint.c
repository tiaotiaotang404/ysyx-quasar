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
//监视点的实现
#include "sdb.h"

#define NR_WP 32  //监视点个数
/*
typedef struct watchpoint {
  int NO; //监视点的序号
  struct watchpoint *next;//指向下一个监视点
  char *expr;//输入的字符串
  long int old_val;

} WP;
*/
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
//链表，head用于组织使用中的监视点结构, free_用于组织空闲的监视点结构

void init_wp_pool() { //对两个链表进行初始化
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;//编号
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);//将结点连接成链表
    wp_pool[i].expr = NULL;//输入的监视点表达式
    wp_pool[i].old_val = 0;//表达式的结果值
  }

  head = NULL;//使用中的监视点链表
  free_ = wp_pool;//空闲监视点链表
}

/* TODO: Implement the functionality of watchpoint */
/**
 * @param expr: const char input
 * @return return the number of allocated watchpoint
*/
//new_wp()从free_链表中返回一个空闲的监视点结构
//将空闲的监视点链表free_第一个取下来，放到使用的监视点链表head的最后
WP* new_wp(){
	WP *temp;
	temp = free_;
	free_ = free_->next;
	temp->next = NULL;//将空闲监视点链表第一个结点取出
	if (head == NULL){
		head = temp;//若head为空，直接相等
	} else {
		WP* temp2;
		temp2 = head;
		while (temp2->next != NULL){
			temp2 = temp2->next;
		}
		temp2->next = temp;//若head不为空，找到head最后，将结点插入到最后
	}
	return temp;
}
/*
WP* wp_new( char * expr_str, bool *success) {
  static int count = 0; // not re-use NO. to avoid confusion
  WP* newnode = free_;//创建一个可移动结点，指向free_链表，用于找到该链表中的某个结点
  word_t expr_val;//输入的监视点地址
  bool _success;
  if (free_ == NULL) {//链表为空
    printf("No available node in watchpoint pool\n");
    assert(0);
  }
  expr_val = expr(expr_str, &_success);
  if (!_success) {
    *success = false;
    return 0;
  }
  free_ = free_->next;
  newnode->next = head;
  head = newnode;
  newnode->NO = count ++;
  newnode->expr = (char*)malloc(strlen(expr_str) + 1);//创建一个结点存储将要被返回的空闲监视点内容
  strcpy(newnode->expr, expr_str);
  newnode->old_val = expr_val;
  *success = true;
  return newnode;//将监视点内容取出来存入newnode
}
*/
/**
 * @param NO: allocated NO for the designated watchpoint
 */
//free_wp()将指定的监视点归还到free_链表中
//当某个监视点使用完毕后，从head中取下指定的结点返还到free_中
void free_wp(WP *wp){
	if (wp == NULL){
		assert(0);
	}
	if (wp == head){
		head = head->next;
	} else {
		WP* temp = head;
		while (temp != NULL && temp->next != wp){
			temp = temp->next;
		}
		temp->next = temp->next->next;
	}
	wp->next =free_;
	free_ = wp;
	wp->old_val = 0;
	//wp->expr[0] = '0';
}
/*
bool wp_free(int NO) {
  WP* ptr = head;
  WP* pre = NULL;
  bool found = false;
  while (ptr) {  //在head中找到wp
    if (ptr->NO == NO) {
      found = true;
      break;
    } else {
      pre = ptr;
      ptr = ptr->next;
    }
  }
  if (found) {
    free(ptr->expr); //清空wp
    ptr->old_val = 0;
    // remember to free memory
    if (ptr != head) {//头插法wp插入_free
      pre->next = ptr->next;
      ptr->next = free_->next;
      free_->next = ptr->next;
    } else {
      head = head->next;
      ptr->next = free_->next;
      free_->next = ptr->next;
    }
  }
  return found;
}
*/


void wp_print() {
  WP* ptr = head;
  if (ptr == NULL) {
    printf("No watchpoint\n");
  }
  else {
    printf("|\tNO\t|     Old Value      |\t Expr\n");
    while (ptr) {
      printf("|\t%d\t|%20ld|  %s\n", ptr->NO, ptr->old_val, ptr->expr);
      ptr = ptr->next;
    }
  }
}

bool wp_check() {
  WP* ptr = head;
  word_t new_val;
  bool success;
  bool is_diff = false;
  while (ptr) {
    //printf("00000000000000\n");
    new_val = expr(ptr->expr, &success);
    //printf("00000000000000\n");
    if (success) {
      //printf("1111111111111111111\n");
      if (new_val != ptr->old_val) {
        //printf("22222222222222222222\n");
        printf("Watchpoint NO %d triggered: \n %s : %ld -> %ld\n", ptr->NO, ptr->expr, ptr->old_val, new_val);
        ptr->old_val = new_val;
        is_diff = true;
      }
    } else {
      //printf("333333333333333333\n");
      printf("Watchpoint NO %d : \'%s\' failed to evaluate\n Execution will pause\n", ptr->NO, ptr->expr);
      return true; // pause nemu, let user decide whether to delete this watch
    }
    //printf("444444444444444\n");
    ptr = ptr->next;
  }
  return is_diff;
}

WP* delete_wp(int p, bool *key){
	WP *temp = head;
	while (temp != NULL && temp->NO != p){
		temp = temp->next;
	}
	if (temp == NULL){
		*key = false;
	}
	return temp;
}