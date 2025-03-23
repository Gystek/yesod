#ifndef YESOD_VM_
# define YESOD_VM_

# include <stdint.h>
# include <stdio.h>
# include "mem.h"

static uint8_t YESOD_VERSION = 0;

struct yesod_vm {
  uint32_t		x0;
  uint32_t		x1;
  uint32_t		x2;
  uint32_t		x3;
  uint32_t		x4;
  uint32_t		x5;
  uint32_t		x6;
  uint32_t		x7;
  uint32_t		x8;
  uint32_t		x9;
  uint32_t		x10;
  uint32_t		x11;
  uint32_t		x12;
  uint32_t		x13;
  uint32_t		pc; /* x14 */
  uint32_t		sp; /* x15 */

  struct yesod_mem	memory;

  /* constants */
  uint32_t	heap;
  uint32_t	rodata;
  uint32_t	data;
  uint32_t	text;
};

int yesod_init_vm (struct yesod_vm *, uint32_t, uint32_t);
int yesod_init_prog (struct yesod_vm *, FILE *);

#endif /* YESOD_VM_ */
