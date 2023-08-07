#include "MBRZA1H.h"
#include <stdlib.h>

IRQHandler _kernel_inh_table[RZ_A1_IRQ_MAX+1];
uint32_t _kernel_excpt_nest_count = 0;

void _kernel_default_exc_handler()
{
}

void _kernel_undef_handler()
{
}

void _kernel_svc_handler()
{
}

void _kernel_pabort_handler()
{
}

void _kernel_dabort_handler()
{
}

void _kernel_irq_handler()
{
}

void _kernel_fiq_handler()
{
}

void __libc_init_array()
{
}

void __libc_fini_array()
{
}

void *memset(void *dst, int c, size_t size)
{
	return NULL;
}

void *memcpy(void *dst, void * src, size_t size)
{
	return NULL;
}

int main()
{
}

void exit(int exitcd)
{
	while(1);
}

void *calloc(size_t a, size_t b)
{
	return NULL;
}

long SYS_futex(long a, long b, long c, long d, long e, long f) {
	return -1;
}
