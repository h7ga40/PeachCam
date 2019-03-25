#include "pthread_impl.h"
#include <threads.h>
#include "libc.h"

static pthread_t __pthread_self_internal()
{
	return __pthread_self();
}

#ifndef __c2__
weak_alias(__pthread_self_internal, pthread_self);
weak_alias(__pthread_self_internal, thrd_current);
#else
pthread_t pthread_self()
{
	return __pthread_self_internal();
}

pthread_t thrd_current()
{
	return __pthread_self_internal();
}
#endif
