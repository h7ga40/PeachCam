#include <stdlib.h>
#include <stdint.h>
#include "libc.h"

#ifndef __c2__
static void dummy()
{
}

/* atexit.c and __stdio_exit.c override these. the latter is linked
 * as a consequence of linking either __toread.c or __towrite.c. */
weak_alias(dummy, __funcs_on_exit);
weak_alias(dummy, __stdio_exit);
weak_alias(dummy, _fini);
#else
__attribute__((weak))
void __funcs_on_exit() { }
void __stdio_exit();
__attribute__((weak))
void _fini() { }
#endif

__attribute__((__weak__, __visibility__("hidden")))
extern void (*const __fini_array_start)(void), (*const __fini_array_end)(void);

static void libc_exit_fini(void)
{
	uintptr_t a = (uintptr_t)&__fini_array_end;
	for (; a>(uintptr_t)&__fini_array_start; a-=sizeof(void(*)()))
		(*(void (**)())(a-sizeof(void(*)())))();
	_fini();
}

#ifndef __c2__
weak_alias(libc_exit_fini, __libc_exit_fini);
#else
void __libc_exit_fini(void);
#endif

_Noreturn void exit(int code)
{
	__funcs_on_exit();
	__libc_exit_fini();
	__stdio_exit();
	_Exit(code);
}
