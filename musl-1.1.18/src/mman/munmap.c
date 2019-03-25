#include <sys/mman.h>
#include "syscall.h"
#include "libc.h"

#ifndef __c2__
static void dummy(void) { }
weak_alias(dummy, __vm_wait);
#else
extern void __vm_wait(void);
#endif

int __munmap(void *start, size_t len)
{
	__vm_wait();
	return syscall(SYS_munmap, start, len);
}

#ifndef __c2__
weak_alias(__munmap, munmap);
#else
int munmap(void *start, size_t len)
{
	return __munmap(start, len);
}
#endif
