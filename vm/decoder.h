#ifndef YESOD_DECODER_
# define YESOD_DECODER_

# include <stdbool.h>
# include <stdint.h>

/* shift types
 *
 * 00 - no shift
 * 01 - left shift
 * 10 - right shift
 * 11 - signed right shift
 */
enum shift {
  NONE = 0b00,
  LSL  = 0b01,
  LSR  = 0b10,
  ASR  = 0b11
};

/*
 * operation size 
 *
 * 00 - word
 * 01 - day
 * 10 - half
 * 11 - byte
 */
enum op_size {
  WORD = 0b00,
  DAY  = 0b01,
  HALF = 0b10,
  BYTE = 0b11
};

/*
 * conditions
 * 000 - always
 * 001 - not equal
 * 010 - less than, unsigned
 * 011 - greater or equal, unsigned
 * 100 - equal
 * 110 - less than, signed
 * 111 - greater or equal, signed
 */
enum cond {
  ALW = 0b000,
  NEQ = 0b001,
  LTU = 0b010,
  GEU = 0b011,
  EEQ = 0b100,
  LTS = 0b110,
  GES = 0b111,
};

enum opcode {
  NOP = 0x00, /* I */
  MOV = 0x01, /* I & II */
  ADD = 0x02, /* I & II */
  SUB = 0x03, /* I & II */
  AND = 0x04, /* I & II */
  OR  = 0x05, /* I & II */
  XOR = 0x06, /* I & II */
  CAR = 0x07, /* I & II */
  CDR = 0x08, /* I & II */
  STR = 0x09, /* I & II */
  JA  = 0x0A, /* III & IV */
  JR  = 0x0B, /* III & IV */
  HLT = 0x0C, /* I */
  CMP = 0x0D, /* I & II */
};

/*
 * | 0 | 1 |  2..7  | 8..11 | 12..13 | 14..15 | 16..19 | 20..22 |   23   | 24..27/24..28 | 28/29..31 |
 * | 0 | 0 | opcode |  rd   | shift  |  size  |   rs   |  cond  | shifti |    rh/imm     |     -     |
 *
 * if `shifti` is set, 25..29 are interpreted as an immediate 5-bit
 * value, if not, 25..28 are interpreted as a register. the resulting
 * value is applied for shifting
 */
struct yesod_instruction1 {
  enum opcode	opcode;
  uint8_t	rd;
  enum shift	shift;
  enum op_size	size;
  uint8_t	rs;
  enum cond	cond;
  bool		shifti;
  union {
    uint8_t	rh;
    uint8_t	imm;
  } 		shift_v;
};

/*
 * | 0 | 1 |  2..7  | 8..11 | 12..14 |      15     | 16..31 |
 * | 0 | 1 | opcode |  rd   |  cond  | upper/lower |  imm   |
 *
 *
 * upper = 1, lower = 0
 */
struct yesod_instruction2 {
  enum opcode	opcode;
  uint8_t	rd;
  enum cond	cond;
  bool		uplo;
  uint16_t	imm;
};

/*
 * | 0 | 1 |  2..7  | 8..11 | 12..13 | 14..15 | 16..18 |  19  |   20   | 21..24/21..25 | 24/25..31 |
 * | 1 | 0 | opcode |  rs   | shift  |  size  |  cond  | push | shifti |    rh/imm     |     -     |
 *
 * see class I for the meaning of `shifti` and `rh/imm`
 *
 * push `pc + 4` on the stack if `push` is set
 */
struct yesod_instruction3 {
  enum opcode	opcode;
  uint8_t	rs;
  enum shift	shift;
  enum op_size	size;
  enum cond	cond;
  bool		push;
  bool		shifti;
  union {
    uint8_t	rh;
    uint8_t	imm;
  }		shift_v;
};

/*
 * | 0 | 1 |  2..7  | 8..11 | 12..14 |  15  | 16..31 |
 * | 1 | 1 | opcode |  rp   |  cond  | push |  imm   |
 *
 * jumps to the value `(rp << 16) | imm`
 *
 * push `pc + 4` on the stack if `push` is set
 */
struct yesod_instruction4 {
  enum opcode	opcode;
  uint8_t	rp;
  enum cond	cond;
  bool		push;
  uint16_t	imm;
};

/* instruction classes
 *
 * I	- general instructions - 4 bytes
 * II	- immediate instructions - 4 bytes
 * III	- branching instructions - 4 bytes
 * IV	- immediate branching instructions - 4 bytes
 */
enum yesod_instruction_class {
  INSTR_CLASS1 = 0b00,
  INSTR_CLASS2 = 0b01,
  INSTR_CLASS3 = 0b10,
  INSTR_CLASS4 = 0b11
};

struct yesod_instruction {
  enum yesod_instruction_class	class;
  union {
    struct yesod_instruction1	instr1;
    struct yesod_instruction2	instr2;
    struct yesod_instruction3	instr3;
    struct yesod_instruction4	instr4;
  }				instr;
};

struct yesod_instruction yesod_decode (uint32_t); 

#endif /* YESOD_DECODER_ */
