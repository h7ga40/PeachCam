#include <locale.h>
#include "locale_impl.h"
#include "libc.h"

static const char *dummy(const char *msg, const struct __locale_map *lm)
{
	return msg;
}

#ifndef __c2__
weak_alias(dummy, __lctrans_impl);
#endif

const char *__lctrans(const char *msg, const struct __locale_map *lm)
{
	return __lctrans_impl(msg, lm);
}

const char *__lctrans_cur(const char *msg)
{
	return __lctrans_impl(msg, CURRENT_LOCALE->cat[LC_MESSAGES]);
}
