#include "stdio_impl.h"

#ifndef __c2__
static int dummy(int fd)
{
	return fd;
}

weak_alias(dummy, __aio_close);
#else
extern int __aio_close(int fd);
#endif

int __stdio_close(FILE *f)
{
	return syscall(SYS_close, __aio_close(f->fd));
}
