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

#include "common.h"
#include <difftest-def.h>
#include <sys/prctl.h>
#include <signal.h>

bool gdb_connect_qemu(int);
bool gdb_memcpy_to_qemu(uint32_t, void *, int);
bool gdb_getR_s(union isa_gdb_R_s *);
bool gdb_setR_s(union isa_gdb_R_s *);
bool gdb_si();
void gdb_exit();

void init_isa();

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  assert(direction == DIFFTEST_TO_REF);
  if (direction == DIFFTEST_TO_REF) {
    bool ok = gdb_memcpy_to_qemu(addr, buf, n);
    assert(ok == 1);
  }
}

void difftest_R_cpy(void *dut, bool direction) {
  union isa_gdb_R_s qemu_r;
  gdb_getR_s(&qemu_r);
  if (direction == DIFFTEST_TO_REF) {
    memcpy(&qemu_r, dut, DIFFTEST_R__SIZE);
    gdb_setR_s(&qemu_r);
  } else {
    memcpy(dut, &qemu_r, DIFFTEST_R__SIZE);
  }
}

void difftest_exec(uint64_t n) {
  while (n --) gdb_si();
}

void difftest_init(int port) {
  char buf[32];
  sprintf(buf, "tcp::%d", port);

  int ppid_before_fork = getpid();
  int pid = fork();
  if (pid == -1) {
    perror("fork");
    assert(0);
  }
  else if (pid == 0) {
    // child

    // install a parent death signal in the chlid
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (r == -1) {
      perror("prctl error");
      assert(0);
    }

    if (getppid() != ppid_before_fork) {
      printf("parent has died!\n");
      assert(0);
    }

    close(STDIN_FILENO);
    execlp(ISA_QEMU_BIN, ISA_QEMU_BIN, ISA_QEMU_ARGS "-S", "-gdb", buf, "-nographic",
        "-serial", "none", "-monitor", "none", NULL);
    perror("exec");
    assert(0);
  }
  else {
    // father

    gdb_connect_qemu(port);
    printf("Connect to QEMU with %s successfully\n", buf);

    atexit(gdb_exit);

    init_isa();
  }
}

void difftest_raise_intr(uint64_t NO) {
  printf("raise_intr is not supported\n");
  assert(0);
}
