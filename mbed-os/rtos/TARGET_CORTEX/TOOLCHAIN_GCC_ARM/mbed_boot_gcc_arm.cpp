/* mbed Microcontroller Library
 * Copyright (c) 2018-2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cmsis.h"
#include "platform/mbed_toolchain.h"
#include "mbed_boot.h"
#include "mbed_rtos_storage.h"
#include "cmsis_os2.h"

static osMutexId_t               malloc_mutex_id;
static mbed_rtos_storage_mutex_t malloc_mutex_obj;
static osMutexAttr_t             malloc_mutex_attr;

static osMutexId_t               env_mutex_id;
static mbed_rtos_storage_mutex_t env_mutex_obj;
static osMutexAttr_t             env_mutex_attr;

#if !defined(HEAP_START)
/* Defined by linker script */
extern uint32_t __end__[];
#define HEAP_START      ((unsigned char*)__end__)
#define HEAP_SIZE       ((uint32_t)((uint32_t)INITIAL_SP - (uint32_t)HEAP_START))
#endif

extern "C" void __init_libc(const char *const *const envp, const char *pn);
extern "C" void __libc_start_init(void);

/*
 * mbed entry point for the GCC toolchain
 *
 * Override gcc boot hook software_init_hook to run code before main.
 */
extern "C" void software_init_hook(void)
{
    unsigned char *free_start = HEAP_START;
    uint32_t free_size = HEAP_SIZE;

#ifdef ISR_STACK_START
    /* Interrupt stack explicitly specified */
    mbed_stack_isr_size = ISR_STACK_SIZE;
    mbed_stack_isr_start = ISR_STACK_START;
#else
    /* Interrupt stack -  reserve space at the end of the free block */
    mbed_stack_isr_size = ISR_STACK_SIZE < free_size ? ISR_STACK_SIZE : free_size;
    mbed_stack_isr_start = free_start + free_size - mbed_stack_isr_size;
    free_size -= mbed_stack_isr_size;
#endif

    /* Heap - everything else */
    mbed_heap_size = free_size;
    mbed_heap_start = free_start;

	const char *const args[2] = {
		"mbed",
		NULL
	};
	const char *const envs[1] = {
		NULL
	};
	__init_libc(envs, args[0]);

    mbed_init();
    mbed_rtos_start();
}

extern "C" void mbed_toolchain_init()
{
    malloc_mutex_attr.name = "malloc_mutex";
    malloc_mutex_attr.attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust;
    malloc_mutex_attr.cb_size = sizeof(malloc_mutex_obj);
    malloc_mutex_attr.cb_mem = &malloc_mutex_obj;
    malloc_mutex_id = osMutexNew(&malloc_mutex_attr);

    env_mutex_attr.name = "env_mutex";
    env_mutex_attr.attr_bits = osMutexRecursive | osMutexPrioInherit | osMutexRobust;
    env_mutex_attr.cb_size = sizeof(env_mutex_obj);
    env_mutex_attr.cb_mem = &env_mutex_obj;
    env_mutex_id = osMutexNew(&env_mutex_attr);

    /* Run the C++ global object constructors */
	__libc_start_init();
}

extern "C" int __real_main(void);
extern "C" int __wrap_main(void) {
    /* For backwards compatibility */
    return __real_main();
}

/* Opaque declaration of _reent structure */
struct _reent;

extern "C" void __rtos_malloc_lock( struct _reent *_r )
{
	(void)_r;
    osMutexAcquire(malloc_mutex_id, osWaitForever);
}

extern "C" void __rtos_malloc_unlock( struct _reent *_r )
{
	(void)_r;
    osMutexRelease(malloc_mutex_id);
}

extern "C" void __rtos_env_lock( struct _reent *_r )
{
	(void)_r;
    osMutexAcquire(env_mutex_id, osWaitForever);
}

extern "C" void __rtos_env_unlock( struct _reent *_r )
{
	(void)_r;
    osMutexRelease(env_mutex_id);
}
