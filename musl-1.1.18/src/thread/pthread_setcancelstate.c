#include "pthread_impl.h"

int __pthread_setcancelstate(int new, int *old)
{
	if (new > 2U) return EINVAL;
	struct pthread *self = __pthread_self();
	if (old) *old = self->canceldisable;
	self->canceldisable = new;
	return 0;
}

#ifndef __c2__
weak_alias(__pthread_setcancelstate, pthread_setcancelstate);
#else
int pthread_setcancelstate(int new, int *old)
{
	return __pthread_setcancelstate(new, old);
}
#endif
