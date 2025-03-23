#ifndef YESOD_VM_
# define YESOD_VM_

# include <stdint.h>
# include <stdio.h>
# include "mem.h"

#define YESOD_VERSION (0)

# define PC (14)
# define SP (15)

struct yesod_vm {
  /* register x0 holds the 0 value at all times */
  /* pc in x14 and sp in x15 */
  uint32_t		regs[16];

  struct yesod_mem	memory;

  /* constants */
  uint32_t	heap;
  uint32_t	rodata;
  uint32_t	data;
  uint32_t	text;

  /* flags */
  /*
   * 0 - nil
   * 1 - carry
   * 2 - sign
   * 3 - overflow
   * 4..7 - reserved
   */
  uint8_t	flags;
};

# define FLAG_NIL   (0b00000001)
# define FLAG_CARRY (0b00000010)
# define FLAG_SIGN  (0b00000100)
# define FLAG_OVER  (0b00001000)

int	yesod_init_vm (struct yesod_vm *, uint32_t, uint32_t);
int	yesod_init_prog (struct yesod_vm *, FILE *);
void	yesod_dump_vm (struct yesod_vm *);
void	yesod_destroy_vm (struct yesod_vm *);

#endif /* YESOD_VM_ */
