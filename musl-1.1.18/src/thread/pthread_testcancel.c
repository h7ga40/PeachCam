#include "pthread_impl.h"
#include "libc.h"

static void dummy()
{
}

weak_alias(dummy, __testcancel);

void __pthread_testcancel()
{
	__testcancel();
}

#ifndef __c2__
weak_alias(__pthread_testcancel, pthread_testcancel);
#else
void pthread_testcancel()
{
	__pthread_testcancel();
}
#endif
