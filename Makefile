MOD_SRC := $(shell pwd)

obj-m := simple_mod.o

simple_mod-y := \
	simple_driver.o \
	simple_context.o

ccflags-y += -Wall -Wextra -Werror

default: mod

mod:
	${MAKE} -C ${KERNEL_SOURCE} M=${MOD_SRC} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${MOD_SRC} clean

format:
	clang-format -i *.c *.h
