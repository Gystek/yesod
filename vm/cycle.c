#include "cycle.h"
#include "decoder.h"

static uint32_t
fetch (vm)
     struct yesod_vm *vm;
{
  uint32_t pc = vm->regs[PC];
  uint8_t x0 = vm->memory.memory[pc];
  uint8_t x1 = vm->memory.memory[pc + 1];
  uint8_t x2 = vm->memory.memory[pc + 2];
  uint8_t x3 = vm->memory.memory[pc + 3];
  
  return ((uint32_t)x0
          | ((uint32_t)x1 << 8)
          | ((uint32_t)x2 << 16)
          | ((uint32_t)x3 << 24));
}

static bool
check (vm, cond)
     struct yesod_vm    *vm;
     enum cond          cond;
{
  switch (cond)
    {
    case ALW:
      return true;
    case NEQ:
      return (vm->flags & FLAG_NIL);
    case EEQ:
      return !(vm->flags & FLAG_NIL);
    case LTU:
      return (vm->flags & FLAG_CARRY);
    case GEU:
      return !(vm->flags & FLAG_CARRY);
    case LTS:
      return !(vm->flags & FLAG_OVER);
    case GES:
      return (vm->flags & FLAG_OVER);
    }

  return true;
}

static uint32_t
shift (x, st, s)
     uint32_t   x;
     enum shift st;
     uint8_t    s;
{
  switch (st)
    {
    case NONE:
      return x;
    case LSL:
      return (x << (uint32_t)s);
    case LSR:
      return (x >> (uint32_t)s);
    case ASR:
      return ((uint32_t)((int32_t)x >> (int32_t)s));
    }

  return x;
}

static uint32_t
fit (x, s)
     uint32_t           x;
     enum op_size       s;
{
  switch (s)
    {
    case WORD:
      return x;
    case DAY:
      return (x & 0b00000000111111111111111111111111);
    case HALF:
      return (x & 0b00000000000000001111111111111111);
    case BYTE:
      return (x & 0b00000000000000000000000011111111);
    }

  return x;
}

static void
partial_flagset (vm, r)
     struct yesod_vm    *vm;
     uint8_t            r;
{
  if (!vm->regs[r])
    vm->flags |= FLAG_NIL;

  if ((vm->regs[r] & 0b10000000000000000000000000000000))
    vm->flags |= FLAG_SIGN;
}

static void
mov (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] = x;
  partial_flagset (vm, rd);
}

#define HALF_UINT32_T (0xFFFFFFFF >> 1)

static bool
overflow_add (x, y)
     uint32_t x, y;
{
  int32_t sx = *(int32_t *)&x;
  int32_t sy = *(int32_t *)&y;
  int64_t lsx = (int64_t)sx;
  int64_t lsy = (int64_t)sy;

  int64_t rl = lsx + lsy;
  int64_t rf = (int64_t)(sx + sy);

  return (rl != rf);
}

static void
add (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  if (overflow_add (vm->regs[rd], x))
    vm->flags |= FLAG_OVER;

  if (x > HALF_UINT32_T && vm->regs[rd] > HALF_UINT32_T)
    vm->flags |= FLAG_CARRY;

  vm->regs[rd] += x;
  partial_flagset (vm, rd);
}

static bool
overflow_sub (x, y)
     uint32_t x, y;
{
  int32_t sx = *(int32_t *)&x;
  int32_t sy = *(int32_t *)&y;
  int64_t lsx = (int64_t)sx;
  int64_t lsy = (int64_t)sy;

  int64_t rl = lsx - lsy;
  int64_t rf = (int64_t)(sx - sy);

  return (rl != rf);
}

static void
sub (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  if (overflow_sub (vm->regs[rd], x))
    vm->flags |= FLAG_OVER;

  if (x < vm->regs[rd])
    vm->flags |= FLAG_CARRY;

  vm->regs[rd] -= x;
  partial_flagset (vm, rd);
}

static void
and (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] &= x;
  partial_flagset (vm, rd);
}

static void
or (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] |= x;
  partial_flagset (vm, rd);
}

static void
xor (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] ^= x;
  partial_flagset (vm, rd);
}

static void
car (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] = vm->memory.memory[x];
  partial_flagset (vm, rd);
}

static void
cdr (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->regs[rd] = vm->memory.memory[x + sizeof(uint32_t)];
  partial_flagset (vm, rd);
}

static void
str (vm, rd, x)
     struct yesod_vm	*vm;
     uint8_t		rd;
     uint32_t		x;
{
  vm->memory.memory[vm->regs[rd]] = x;
}

static void
cmp (vm, rx, y)
     struct yesod_vm	*vm;
     uint8_t		rx;
     uint32_t		y;
{
  vm->regs[0] = vm->regs[rx];

  return sub(vm, 0, y);
}

static uint32_t
cycle1 (vm, instr)
     struct yesod_vm		*vm;
     struct yesod_instruction1	instr;
{
  uint32_t src = vm->regs[instr.rs];

  if (!check (vm, instr.cond))
    return 0;

  uint8_t sh = (instr.shifti ? instr.shift_v.imm : (uint8_t)vm->regs[instr.shift_v.rh]);

  src = shift (src, instr.shift, sh);

  src = fit (src, instr.size);

  switch (instr.opcode)
    {
    case NOP:
      return 0;
    case HLT:
      return src;
    case MOV:
      mov (vm, instr.rd, src);
      return 0;
    case ADD:
      add (vm, instr.rd, src);
      return 0;
    case SUB:
      sub (vm, instr.rd, src);
      return 0;
    case AND:
      and (vm, instr.rd, src);
      return 0;
    case OR:
      or (vm, instr.rd, src);
      return 0;
    case XOR:
      xor (vm, instr.rd, src);
      return 0;
    case CAR:
      car (vm, instr.rd, src);
      return 0;
    case CDR:
      cdr (vm, instr.rd, src);
      return 0;
    case STR:
      str (vm, instr.rd, src);
      return 0;
    case CMP:
      cmp (vm, instr.rd, src);
      return 0;
    default:
      return 1;
    }
}

static uint32_t
cycle2 (vm, instr)
     struct yesod_vm		*vm;
     struct yesod_instruction2	instr;
{
  uint32_t src = (uint32_t)instr.imm;

  if (!check (vm, instr.cond))
    return 0;

  if (instr.uplo)
    src <<= 16;

  switch (instr.opcode)
    {
    case MOV:
      mov (vm, instr.rd, src);
      return 0;
    case ADD:
      add (vm, instr.rd, src);
      return 0;
    case SUB:
      sub (vm, instr.rd, src);
      return 0;
    case AND:
      and (vm, instr.rd, src);
      return 0;
    case OR:
      or (vm, instr.rd, src);
      return 0;
    case XOR:
      xor (vm, instr.rd, src);
      return 0;
    case CAR:
      car (vm, instr.rd, src);
      return 0;
    case CDR:
      cdr (vm, instr.rd, src);
      return 0;
    case STR:
      str (vm, instr.rd, src);
      return 0;
    case CMP:
      cmp (vm, instr.rd, src);
      return 0;
    default:
      return 1;
    }
}

static int
push (vm, x)
     struct yesod_vm	*vm;
     uint32_t		x;
{
  uint32_t sp = vm->regs[SP];

  if (sp - STACK > vm->memory.s_size)
    return 1;

  vm->memory.memory[sp] = (uint8_t)x;
  vm->memory.memory[sp + 1] = (uint8_t)(x >> 8);
  vm->memory.memory[sp + 2] = (uint8_t)(x >> 16);
  vm->memory.memory[sp + 3] = (uint8_t)(x >> 24);

  vm->regs[SP] += 4;

  return 0;
}

static uint32_t
cycle3 (vm, instr)
     struct yesod_vm		*vm;
     struct yesod_instruction3	instr;
{
  uint32_t src = vm->regs[instr.rs];

  if (!check (vm, instr.cond))
    return 0;

  uint8_t sh = (instr.shifti ? instr.shift_v.imm : (uint8_t)vm->regs[instr.shift_v.rh]);

  src = shift (src, instr.shift, sh);

  src = fit (src, instr.size);

  if (instr.push)
    if (push (vm, vm->regs[PC]))
      return 1;

  switch (instr.opcode)
    {
    case JA:
      vm->regs[PC] = src;
      return 0;
    case JR:
      vm->regs[PC] += src - 4 /* we've already incremented pc */;
      return 0;
    default:
      return 1;
    }
}

static uint32_t
cycle4 (vm, instr)
     struct yesod_vm		*vm;
     struct yesod_instruction4	instr;
{
  uint32_t src = instr.imm;

  if (!check (vm, instr.cond))
    return 0;

  src |= (vm->regs[instr.rp] << 16);
  
  if (instr.push)
    if (push (vm, vm->regs[PC]))
      return 1;

  switch (instr.opcode)
    {
    case JA:
      vm->regs[PC] = src;
      return 0;
    case JR:
      vm->regs[PC] += src - 4 /* we've already incremented pc */;
      return 0;
    default:
      return 1;
    }
}

uint32_t
yesod_cycle (vm)
     struct yesod_vm *vm;
{
  uint32_t			raw;
  struct yesod_instruction	instr;

  raw = fetch (vm);
  instr = yesod_decode (raw);

  /* reset x0 to 0 before every cycle */
  vm->regs[0] = 0;
  vm->regs[PC] += 4;

  switch (instr.class)
    {
    case INSTR_CLASS1:
      return cycle1 (vm, instr.instr.instr1);
    case INSTR_CLASS2:
      return cycle2 (vm, instr.instr.instr2);
    case INSTR_CLASS3:
      return cycle3 (vm, instr.instr.instr3);
    case INSTR_CLASS4:
      return cycle4 (vm, instr.instr.instr4);
    }

  return 0;
}
