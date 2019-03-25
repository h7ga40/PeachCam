#include <sys/mman.h>
#include "libc.h"
#include "syscall.h"

int __mprotect(void *addr, size_t len, int prot)
{
	size_t start, end;
	start = (size_t)addr & -PAGE_SIZE;
	end = (size_t)((char *)addr + len + PAGE_SIZE-1) & -PAGE_SIZE;
	return syscall(SYS_mprotect, start, end-start, prot);
}

#ifndef __c2__
weak_alias(__mprotect, mprotect);
#else
int mprotect(void *addr, size_t len, int prot)
{
	return __mprotect(addr, len, prot);
}
#endif
