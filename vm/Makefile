# Makefile for the Yesod emulator
CC := gcc
LD := $(CC)
CFLAGS := -ansi -Wall -Wextra -Wwrite-strings -Wno-variadic-macros
LDFLAGS :=

CSRC := main.c vm.c decoder.c cycle.c
COBJ := $(CSRC:.c=.o)

all: yesod-vm
yesod-vm: $(COBJ)
	$(LD) -o $@ $^

clean:
	rm -f $(COBJ)
	rm -f yesod-vm

.PHONY: all clean
