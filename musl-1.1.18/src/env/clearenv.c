#define _GNU_SOURCE
#include <stdlib.h>
#include "libc.h"

#ifndef __c2__
static void dummy(char *old, char *new) {}
weak_alias(dummy, __env_rm_add);
#else
extern void __env_rm_add(char *old, char *new);
#endif

int clearenv()
{
	char **e = __environ;
	__environ = 0;
	if (e) while (*e) __env_rm_add(*e++, 0);
	return 0;
}
