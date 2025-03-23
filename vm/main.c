#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vm.h"
#include "cycle.h"

int
main (argc, argv)
     int argc;
     char *const argv[];
{
  struct yesod_vm	vm;
  int			opt;
  uint32_t		mem = 4096, stack = 32 * 4;
  uint32_t		ret;
  FILE			*f;

  while ((opt = getopt (argc, argv, "m:s:")) != -1)
    {
      switch (opt)
	{
	case 'm':
	  mem = strtoul (optarg, NULL, 10);
	  break;
	case 's':
	  stack = strtoul (optarg, NULL, 10);
	  break;
	default:
	  fprintf (stderr, "usage: %s [-m mem] [-s stack] file\n", argv[0]);
	  return EXIT_FAILURE;
	}
    }

  if (optind >= argc)
    {
      fprintf (stderr, "usage: %s [-m mem] [-s stack] file\n", argv[0]);
      return EXIT_FAILURE;
    }

  f = fopen (argv[optind], "r");
  if (!f)
    {
      perror ("yesod");
      return EXIT_FAILURE;
    }

  if (yesod_init_vm (&vm, mem, stack))
    return EXIT_FAILURE;

  if (yesod_init_prog (&vm, f))
    {
      yesod_destroy_vm (&vm);

      return EXIT_FAILURE;
    }

  while (!(ret = yesod_cycle (&vm)))
	 ;

  yesod_dump_vm (&vm);

  printf ("exit code: %u\n", ret);

  yesod_destroy_vm (&vm);

  return EXIT_SUCCESS;
}
