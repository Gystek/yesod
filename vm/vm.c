#include <stdlib.h>
#include "vm.h"

int
yesod_init_vm (vm, mem, stack)
     struct yesod_vm	*vm;
     uint32_t		mem;
     uint32_t		stack;
{
  struct yesod_mem memory;

  memory.m_size = mem;
  memory.s_size = stack;
  memory.memory = malloc (mem);

  if (!memory.memory)
    return 1;

  vm->memory = memory;
  vm->flags = 0;

  printf ("yesod: initialised VM with %u bytes of memory (%u bytes stack)\n",
	  mem, stack);

  return 0;
}

#define ARRAY_TO_UINT32_T(a) ((uint32_t)a[0] | ((uint32_t)a[1] << 8)\
			      | ((uint32_t)a[2] << 16) | ((uint32_t)a[3] << 24))

/*
 * everything is LE
 * 
 * |--------------------------------------------|
 * | 0..4 |   5..8    |    9..12    |  13..16   |
 * | YSWD | bin_size  |  text_size  | data_size |
 * |--------------------------------------------|
 * |    17..20   |   21    |  22..22+text_size  |
 * | rodata_size | version |        code        |
 * |--------------------------------------------|
 * |   22+text_size..22+text_size+rodata_size   |
 * |                   rodata                   |
 * |--------------------------------------------|
 * |  22+text+rodata..22+text+rodata+data_size  |
 * |                    data                    |
 * |--------------------------------------------|
 * |     22+text_size+rodata_size+data_size     |
 * |                      0                     |
 * |--------------------------------------------|
 */
int
yesod_init_prog (vm, f)
     struct yesod_vm	*vm;
     FILE		*f;
{
  uint8_t	buffer[4], version;
  uint32_t	size, t_size, r_size, d_size;

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  if (buffer[0] != 'Y'
      || buffer[1] != 'S'
      || buffer[2] != 'W'
      || buffer[3] != 'D')
    {
      fprintf (stderr, "yesod: invalid magic number\n");
      return 1;
    }

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  size = ARRAY_TO_UINT32_T(buffer);

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  t_size = ARRAY_TO_UINT32_T(buffer);
  vm->text = vm->memory.m_size - t_size;

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  d_size = ARRAY_TO_UINT32_T(buffer);
  vm->data = vm->text - d_size;

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  r_size = ARRAY_TO_UINT32_T(buffer);
  vm->rodata = vm->data - r_size;

  if (t_size + d_size + r_size != size - 22)
    {
      fprintf (stderr, "yesod: announced binary size (%u) incoherent with section sizes (%u)\n",
	       size, t_size + d_size + r_size);

      return 1;
	       
    }

  if (t_size + d_size + r_size + vm->memory.s_size > vm->memory.m_size)
    {
      fprintf (stderr, "yesod: binary too large (%u bytes) for allocated memory (%u)\n",
	       t_size + d_size + r_size, vm->memory.m_size);

      return 1;
    }

  if (fread (buffer, 1, 4, f) != 4)
    {
      perror ("yesod");
      return 1;
    }

  version = ARRAY_TO_UINT32_T(buffer);

  if (version != YESOD_VERSION)
    {
      fprintf (stderr, "yesod: program version (%u) incoherent with emulator version (%u)\n",
	       version, YESOD_VERSION);

      return 1;
    }

  if (fread (vm->memory.memory + vm->text, 1, t_size, f) != t_size)
    {
      perror ("yesod");
      return 1;
    }

    if (fread (vm->memory.memory + vm->rodata, 1, r_size, f) != r_size)
    {
      perror ("yesod");
      return 1;
    }

    if (fread (vm->memory.memory + vm->data, 1, d_size, f) != d_size)
    {
      perror ("yesod");
      return 1;
    }

    buffer[0] = 1;

    if (fread (buffer, 1, 1, f) != 1)
      {
	perror ("yesod");
	return 1;
      }

    if (buffer[0] != 0)
      {
	fprintf (stderr, "yesod: missing zero terminator at binary file end\n");

	return 1;
      }

    vm->heap = vm->memory.s_size;

    printf ("yesod: program initialised succesfully\n");
    printf ("  stack\t%#010x\n", 0);
    printf ("  heap\t%#010x\n", vm->heap);
    printf ("  .data\t%#010x\n", vm->data);
    printf ("  .rodata\t%#010x\n", vm->rodata);
    printf ("  .text\t%#010x\n", vm->text);

    vm->regs[PC] = vm->text;
    vm->regs[SP] = 0;

    return 0;
}
