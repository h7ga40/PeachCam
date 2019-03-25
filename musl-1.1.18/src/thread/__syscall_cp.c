#include "pthread_impl.h"
#include "syscall.h"

#undef sccp
__attribute__((__visibility__("hidden")))
long __syscall_cp_c();

static long sccp(syscall_arg_t nr,
                 syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                 syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
	return (__syscall_nr)(nr, u, v, w, x, y, z);
}

#ifndef __c2__
weak_alias(sccp, __syscall_cp_c);
#else
long __syscall_cp_c(syscall_arg_t nr,
	syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
	syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
	return sccp(nr, u, v, w, x, y, z);
}
#endif

long (__syscall_cp)(syscall_arg_t nr,
                    syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                    syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
	return __syscall_cp_c(nr, u, v, w, x, y, z);
}
