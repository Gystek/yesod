#include "decoder.h"

#define OPCODE(x) ((x &         0b11111100) >> 2)
#define REG1(x)   ((x &     0b111100000000) >> 8)
#define SHIFT(x)  ((x &   0b11000000000000) >> 12)
#define SIZE(x)   ((x & 0b1100000000000000) >> 14)

#define RS_1(x)     ((x &     0b11110000000000000000) >> 16)
#define COND_1(x)   ((x &  0b11100000000000000000000) >> 20)
#define SHIFTI_1(x) ((x & 0b100000000000000000000000) >> 23)

#define RH_1(x)  ((x &  0b1111000000000000000000000000) >> 24)
#define IMM_1(x) ((x & 0b11111000000000000000000000000) >> 24)

static struct yesod_instruction1
decode1 (raw)
     uint32_t raw;
{
  struct yesod_instruction1 instr;
  
  instr.opcode = OPCODE(raw);
  instr.rd = REG1(raw);
  instr.shift = SHIFT(raw);
  instr.size = SIZE(raw);
  instr.rs = RS_1(raw);
  instr.cond = COND_1(raw);
  instr.shifti = SHIFTI_1(raw);

  if (instr.shifti)
    instr.shift_v.imm = IMM_1(raw);
  else
    instr.shift_v.rh = RH_1(raw);

  return instr;
}

#define IMM(x) ((x & 0b11111111111111110000000000000000) >> 16)

#define COND_24(x) ((x &  0b111000000000000) >> 12)
#define UPLO(x)    ((x & 0b1000000000000000) >> 15)

static struct yesod_instruction2
decode2 (raw)
     uint32_t raw;
{
  struct yesod_instruction2 instr;

  instr.opcode = OPCODE(raw);
  instr.rd = REG1(raw);
  instr.cond = COND_24(raw);
  instr.uplo = UPLO(raw);
  instr.imm = IMM(raw);

  return instr;
}

#define COND_3(x)   ((x &        0b1110000000000000000) >> 16)
#define PUSH_3(x)   ((x &       0b10000000000000000000) >> 19)
#define SHIFTI_3(x) ((x &      0b100000000000000000000) >> 20)
#define RH_3(x)     ((x &  0b1111000000000000000000000) >> 21)
#define IMM_3(x)    ((x & 0b11111000000000000000000000) >> 21)

static struct yesod_instruction3
decode3 (raw)
     uint32_t raw;
{
  struct yesod_instruction3 instr;

  instr.opcode = OPCODE(raw);
  instr.rs = REG1(raw);
  instr.shift = SHIFT(raw);
  instr.size = SIZE(raw);
  instr.cond = COND_3(raw);
  instr.push = PUSH_3(raw);
  instr.shifti = SHIFTI_3(raw);

  if (instr.shifti)
    instr.shift_v.imm = IMM_3(raw);
  else
    instr.shift_v.rh = RH_3(raw);

  return instr;
}

#define PUSH_4(x) ((x & 0b1000000000000000) >> 15)

static struct yesod_instruction4
decode4 (raw)
     uint32_t raw;
{
  struct yesod_instruction4 instr;

  instr.opcode = OPCODE(raw);
  instr.rp = REG1(raw);
  instr.cond = COND_24(raw);
  instr.push = PUSH_4(raw);
  instr.imm = IMM(raw);

  return instr;
}

#define INSTR_CLASS_MASK (0b11)

struct yesod_instruction
yesod_decode (raw)
     uint32_t raw;
{
  struct yesod_instruction instr;
  enum yesod_instruction_class cl = raw & INSTR_CLASS_MASK;

  instr.class = cl;

  switch (cl)
    {
    case INSTR_CLASS1:
      instr.instr.instr1 = decode1 (raw);
      break;
    case INSTR_CLASS2:
      instr.instr.instr2 = decode2 (raw);
      break;
    case INSTR_CLASS3:
      instr.instr.instr3 = decode3 (raw);
      break;
    case INSTR_CLASS4:
      instr.instr.instr4 = decode4 (raw);
      break;
    }

  return instr;
}
