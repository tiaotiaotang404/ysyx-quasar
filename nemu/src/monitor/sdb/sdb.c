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
// 简易调试器的命令处理
#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
// #include "watchpoint.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  // cpu_exec(1);
  return 0;
}

static int cmd_q(char *args)
{
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args)
{
  // 继续读取得到单步运行的步数，随后调用cpu执行
  // char *num[] = {"1","2","3","4","5","6","7","8","9","10"};
  int num;
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  {
    cpu_exec(1); // 缺省即为1
  }
  else
  {
    // 将arg的内容取出来转换为整数传递给cpu_exec()函数
    /*
    //实现方式一
    for (int i = 0; i < 10; i ++) {
      if (strcmp(arg, num[i]) == 0) {
        cpu_exec(i+1);
        return 0;
      }
    }
    */
    // 实现方式二 没成功
    // strcpy(str,"arg");
    // sscanf("str", "%d", &num);  //用这个函数无法成功将arg的值传递给num
    // 实现方式三
    num = atoi(arg);
    cpu_exec(num);
    // return 0;
    // printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_info(char *args)
{
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  {
    printf("Incomplete command!\n");
  }
  else
  {
    if (strcmp(arg, "r") == 0)
    {
      isa_reg_display();
      return 0;
    }
    else if (strcmp(arg, "w") == 0)
    {
      // 打印监视点信息
      wp_print();
      return 0;
    }
    else
    {
      printf("Unknown command '%s'\n", arg);
    }
  }
  return 0;
}

static int cmd_x(char *args)
{
  // char *num[] = {"1","2","3","4","5","6","7","8","9","10"};
  int offset = 0;
  vaddr_t ini_add; // ini_add 要扫描的初始地址；offset 需要输出的内存字节数
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  {
    printf("Incomplete command!\n");
  }
  else
  {
    // 获取需要输出的内存字节数
    // 实现方式一
    // for (int i = 0; i < 10; i ++) {
    //   if (strcmp(arg, num[i]) == 0) {
    //     offset = i + 1;
    // return 0;
    //  }
    // }
    // 实现方式二
    offset = atoi(arg);
    // 获取被扫描内存的初始地址
    char *arg1 = strtok(NULL, " ");
    if (arg1 == NULL)
    {
      printf("Incomplete command!\n");
    }
    else
    {
      // 实现方式一：接受十进制数字正常，十六进制结果为0
      // ini_add = atoi(arg1);
      // 实现方式二：

      sscanf(arg1, "%lx", &ini_add);
    }
  }
  printf("0x%lx:", ini_add);
  for (int i = 0; i < offset; i++)
  {
    // printf("0x%lx:",ini_add);
    printf("0x%08lx\n", vaddr_read(ini_add, 4));
    ini_add += 4;
    printf("           ");
  }
  printf("\n");
  return 0;
}

static int cmd_p(char *args)
{
  word_t val;
  bool success;
  assert(args != NULL);
  val = expr(args, &success);
  if (success)
    printf("%ld\t\t%016lx\n", val, (uint64_t)val);
  else
    printf("Failed to eval expr\n");
  return 0;
}

static int cmd_w(char *args)
{
  // #ifdef CONFIG_WATCHPOINT
  // bool success;
  WP *wp;
  wp = new_wp();
  /*
  if (!success) {
    printf("Failed to create watchpoint with expr \'%s\'\n", args);
    return 1;
  } else {
    printf("Created watchpoint NO %d, now \'%s\'=%ld\n", wp->NO, wp->expr, wp->old_val);
    return 0;
  }
  */
  // #else
  // printf("watchpoint function turned off\n");
  printf("Created watchpoint NO %d, now \'%s\'=%ld\n", wp->NO, wp->expr, wp->old_val);
  return 0;
  // #endif
}

static int cmd_d(char *args)
{
  int p;
  bool key = true;
  sscanf(args, "%d", &p);
  WP *q = delete_wp(p, &key);
  if (key)
  {
    printf("Delete watchpoint %d: %s\n", q->NO, q->expr);
    free_wp(q);
    return 0;
  }
  else
  {
    printf("No found watchpoint %d\n", p);
    return 0;
  }
  return 0;
}

static int cmd_help(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Single step operation", cmd_si},
    {"info", "Print register", cmd_info},
    {"x", "Scan memory", cmd_x},
    {"p", "Expression evaluation", cmd_p},
    {"w", "Watch expression value, pause when value changed", cmd_w},
    {"d", "Delete watchpoint by its NO", cmd_d}
    /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  if (is_batch_mode)
  {
    cmd_c(NULL); // 指令cmd_c入口
    return;      // 直接结束sdb_mainloop()，返回上一层函数，即engine_start(),最后返回至主函数
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " "); // 以空格作为分隔符，每次以分隔符为开头读取1个字节
    if (cmd == NULL)
    {
      continue;
    } // 查询到字符串末尾时返回NULL，结束本次for循环

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }      // 指令若为cmd_q，直接return至上一层函数
        break; // 找到对应的命令，退出for循环
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    } // 没有找到对应的指令
  }
}

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
