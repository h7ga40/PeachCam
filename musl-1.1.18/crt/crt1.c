#include <features.h>

#define START "musl_start"

#include "crt_arch.h"

int _sta_ker();
void _init() __attribute__((weak));
void _fini() __attribute__((weak));
_Noreturn int __libc_start_main(int (*)(), int, char **,
	void (*)(), void(*)(), void(*)());

void musl_start_c(long *p)
{
	int argc = p[0];
	char **argv = (void *)(p+1);
	__libc_start_main(_sta_ker, argc, argv, _init, _fini, 0);
}
