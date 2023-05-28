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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <../src/utils/itrace.h>
// #include <../src/monitor/ftrace/ftracer.h>

#define R_(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write
#define SetIfCon cmp_and_return
#define Branch branch
#define Csrrs isa_csrrs
#define Csrrw isa_csrrw
#define Ecall ecall_helper

word_t isa_csrrs(word_t rd, word_t src1, word_t csr);
word_t isa_csrrw(word_t rd, word_t src1, word_t csr);
word_t cmp_and_return(uint64_t src1, uint64_t imm, int type);
void branch(uint64_t src1, uint64_t src2, uint64_t imm, Decode *s, int type);
void ecall_helper(Decode *s);

typedef struct
{
  word_t mepc;
  word_t mstatus;
  word_t mcause;
  word_t mtvec;
} riscv64_CSR_state;

extern word_t isa_raise_intr(word_t NO, vaddr_t epc);
riscv64_CSR_state CSRs;

enum
{
  TYPE_I,
  TYPE_U,
  TYPE_S,
  TYPE_N,
  TYPE_J,
  TYPE_B,
  TYPE_R // none
};
enum
{
  Beq = 512,
  Bge,
  Bgeu,
  Blt,
  Bltu,
  Bne,
  Sltu,
  Sltiu,
  Slt,
  Slti
} Instruction_type;

#define src1R()      \
  do                 \
  {                  \
    *src1 = R_(rs1); \
  } while (0)
#define src2R()      \
  do                 \
  {                  \
    *src2 = R_(rs2); \
  } while (0)
#define immI()                        \
  do                                  \
  {                                   \
    *imm = SEXT(BITS(i, 31, 20), 12); \
  } while (0)
#define immU()                              \
  do                                        \
  {                                         \
    *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
  } while (0)
#define immS()                                               \
  do                                                         \
  {                                                          \
    *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
  } while (0)
#define immJ()                                                                                                            \
  do                                                                                                                      \
  {                                                                                                                       \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 30, 21) << 1) | (BITS(i, 20, 20) << 11) | (BITS(i, 19, 12) << 12); \
  } while (0)
#define immB()                                                                                                          \
  do                                                                                                                    \
  {                                                                                                                     \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 30, 25) << 5 | ((BITS(i, 11, 8)) << 1) | ((BITS(i, 7, 7)) << 11); \
  } while (0)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type)
{
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd = BITS(i, 11, 7);
  switch (type)
  {
  case TYPE_I:
    src1R();
    immI();
    break;
  case TYPE_U:
    immU();
    break;
  case TYPE_S:
    src1R();
    src2R();
    immS();
    break;
  case TYPE_J:
    immJ();
    break;
  case TYPE_B:
    src1R();
    src2R();
    immB();
    break;
  case TYPE_R:
    src1R();
    src2R();
    break;
  }
}

static int decode_exec(Decode *s)
{ // 译码
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)         \
  {                                                                  \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
    __VA_ARGS__;                                                     \
  }

  INSTPAT_START();
  // 模式匹配 INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // 指令名称在代码中仅当注释使用, 不参与宏展开
  // 指令类型用于后续译码过程
  // 指令执行操作则是通过C代码来模拟指令执行的真正行为.
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R_(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R_(rd) = SEXT(BITS(imm, 31, 12) << 12, 32));

  INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw, R, R_(rd) = SEXT(BITS(src1 + src2, 31, 0), 32));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R_(rd) = src1 - src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw, R, R_(rd) = SEXT(BITS(src1 - src2, 31, 0), 32));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R_(rd) = SetIfCon(src1, src2, Slt));
  // INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw, R, R_(rd) = SEXT(BITS(src1 + src2, 31, 0), 32));
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R_(rd) = src1 + src2);
  INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw, R, R_(rd) = SEXT(BITS(src1 * src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, R_(rd) = (int64_t)src1 / (int64_t)src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw, R, R_(rd) = SEXT((int32_t)BITS(src1, 31, 0) / (int32_t)BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw, R, R_(rd) = BITS(src1, 31, 0) / BITS(src2, 31, 0));
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, R_(rd) = R_(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw, R, R_(rd) = SEXT((int32_t)BITS(src1, 31, 0) % (int32_t)BITS(src2, 31, 0), 32));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R_(rd) = src1 << BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, R_(rd) = src1 >> BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw, R, R_(rd) = SEXT(BITS(src1, 31, 0) << BITS(src2, 5, 0), 32)); // riscv64
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R_(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R_(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R_(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R_(rd) = SetIfCon(src1, src2, Sltu));
  // INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw, R, R_(rd) = SEXT(BITS(src1, 31, 0) << BITS(src2, 4, 0), 32));//riscv32
  INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw, R, R_(rd) = SEXT(BITS(src1, 31, 0) >> BITS(src2, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw, R, R_(rd) = SEXT((int32_t)(BITS(src1, 31, 0)) >> BITS(src2, 4, 0), 32));
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, R_(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, R_(rd) = src1 % src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw, R, R_(rd) = SEXT(BITS(src1, 31, 0) % BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, R_(rd) = (int64_t)src1 % (int64_t)src2);
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret, R, s->dnpc = CSRs.mepc);

  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R_(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, R_(rd) = Mr(src1 + imm, 8));
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, R_(rd) = s->pc + 4, s->dnpc = (src1 + imm) & (~1));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R_(rd) = SEXT(Mr(src1 + imm, 4), 32));
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R_(rd) = SetIfCon(src1, imm, Sltiu));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R_(rd) = SetIfCon(src1, imm, Slti));
  INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw, I, R_(rd) = SEXT(BITS(src1 + imm, 31, 0), 32));
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R_(rd) = SEXT(BITS(Mr(src1 + imm, 1), 7, 0), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R_(rd) = SEXT(BITS(Mr(src1 + imm, 2), 15, 0), 16)); // 64 or 16 ?
  INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, R_(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R_(rd) = Mr(src1 + imm, 2));
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli, I, R_(rd) = src1 << BITS(imm, 5, 0));
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli, I, R_(rd) = src1 >> BITS(imm, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw, I, R_(rd) = SEXT(BITS(src1, 31, 0) >> BITS(imm, 4, 0), 32)); // not diffset
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R_(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R_(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R_(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R_(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw, I, R_(rd) = SEXT(BITS(src1, 31, 0) << BITS(imm, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw, I, R_(rd) = SEXT((int32_t)(BITS(src1, 31, 0)) >> BITS(imm, 4, 0), 32));
  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs, I, R_(rd) = Csrrs(rd, src1, imm));
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw, I, R_(rd) = Csrrw(rd, src1, imm));
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, I, Ecall(s));
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai, I, R_(rd) = (int64_t)src1 >> BITS(imm, 5, 0));

  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, Branch(src1, src2, imm, s, Bne));
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, Branch(src1, src2, imm, s, Beq));
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, Branch(src1, src2, imm, s, Bge));
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, Branch(src1, src2, imm, s, Blt));
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, Branch(src1, src2, imm, s, Bgeu));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, Branch(src1, src2, imm, s, Bltu));

  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R_(rd) = cpu.pc + 4, s->dnpc = cpu.pc + imm);

  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd, S, Mw(src1 + imm, 8, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, BITS(src2, 31, 0)));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, BITS(src2, 7, 0)));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, BITS(src2, 15, 0)));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R_(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));

  INSTPAT_END();

  R_(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  s->isa.inst.val = inst_fetch(&s->snpc, 4);                // 取指，更新PC
  IFDEF(CONFIG_ITRACE, trace_inst(s->pc, s->isa.inst.val)); // itrace
  return decode_exec(s);                                    // 译码
}

word_t cmp_and_return(uint64_t num1, uint64_t num2, int type)
{
  switch (type)
  {
  case Sltu:
    if (num1 < num2)
      return 1;
    return 0;
    break;
  case Sltiu:
    if (num1 < num2)
      return 1;
    return 0;
  case Slt:
    if ((int64_t)num1 < (int64_t)num2)
      return 1;
    return 0;
    break;
  case Slti:
    if ((int64_t)num1 < (int64_t)num2)
      return 1;
    return 0;
    break;
  default:
    panic("cmp_and_return fuction falut!");
    break;
  }
  return 0;
}

void branch(word_t src1, word_t src2, uint64_t imm, Decode *s, int type)
{
  switch (type)
  {
  case Beq:
    if (src1 == src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  case Bne:
    if (src1 != src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  case Bge:
    if ((int64_t)src1 >= (int64_t)src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  case Blt:
    if ((int64_t)src1 < (int64_t)src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  case Bltu:
    if (src1 < src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  case Bgeu:
    if (src1 >= src2)
    {
      s->dnpc += imm - 4;
    }
    break;
  default:
    break;
  }
}

word_t isa_csrrs(word_t rd, word_t src1, word_t csr)
{
  word_t t = 0;
  switch (csr)
  {
  case 0x341: // mepc
    t = CSRs.mepc;
    CSRs.mepc = src1 | t;
    break;
  case 0x342: // mcause
    t = CSRs.mcause;
    CSRs.mcause = src1 | t;
    break;
  case 0x300: // mstatus
    t = CSRs.mstatus;
    CSRs.mstatus = src1 | t;
    break;
  case 0x305: // mtvec
    t = CSRs.mtvec;
    CSRs.mtvec = src1 | t;
    break;
  default:
    panic("Unkown csr!");
    break;
  }
  // printf("csr: %lx, t: %lx, src1: %lx, rd: %lx, mepc: %lx, mcause: %lx, mstatus: %lx, mtvec: %lx\n", csr, t, src1, rd, CSRs.mepc, CSRs.mcause, CSRs.mstatus, CSRs.mtvec);
  return t;
}

word_t isa_csrrw(word_t rd, word_t src1, word_t csr)
{
  word_t t = 0;
  switch (csr)
  {
  case 0x341: // mepc
    t = CSRs.mepc;
    CSRs.mepc = src1;
    break;
  case 0x342: // mcause
    t = CSRs.mcause;
    CSRs.mcause = src1;
    break;
  case 0x300: // mstatus
    t = CSRs.mstatus;
    CSRs.mstatus = src1;
    break;
  case 0x305: // mtvec
    t = CSRs.mtvec;
    CSRs.mtvec = src1;
    break;
  default:
    panic("Unkown csr!");
    break;
  }
  return t;
}

void ecall_helper(Decode *s)
{

  s->dnpc = isa_raise_intr(0xb, cpu.pc);
  // printf("ECALL from PC %02lx to PC:%02lx\n", cpu.pc, s->dnpc);
}
