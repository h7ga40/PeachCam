#include <unistd.h>
#include <errno.h>
#include "syscall.h"
#include "libc.h"

#ifndef __c2__
static int dummy(int fd)
{
	return fd;
}

weak_alias(dummy, __aio_close);
#else
extern int __aio_close(int fd);
#endif

int close(int fd)
{
	fd = __aio_close(fd);
	int r = __syscall_cp(SYS_close, fd);
	if (r == -EINTR) r = 0;
	return __syscall_ret(r);
}
