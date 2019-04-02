
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include "platform/mbed_toolchain.h"
#include "platform/FileHandle.h"
#include "platform/SingletonPtr.h"
#include "platform/PlatformMutex.h"
#include "platform/mbed_retarget.h"

#define OPEN_MAX         16
#define FILE_HANDLE_RESERVED    ((FileHandle*)0xFFFFFFFF)

using namespace mbed;

unsigned char *mbed_heap_start = 0;
uint32_t mbed_heap_size = 0;

/* newlib has the filehandle field in the FILE struct as a short, so
 * we can't just return a Filehandle* from _open and instead have to
 * put it in a filehandles array and return the index into that array
 */
static FileHandle *filehandles[OPEN_MAX] = { FILE_HANDLE_RESERVED, FILE_HANDLE_RESERVED, FILE_HANDLE_RESERVED };
static char stdio_in_prev[OPEN_MAX];
static char stdio_out_prev[OPEN_MAX];
static SingletonPtr<PlatformMutex> filehandle_mutex;

namespace mbed {
void mbed_set_unbuffered_stream(std::FILE *_file);

void remove_filehandle(FileHandle *file)
{
    filehandle_mutex->lock();
    /* Remove all open filehandles for this */
    for (unsigned int fh_i = 0; fh_i < sizeof(filehandles) / sizeof(*filehandles); fh_i++) {
        if (filehandles[fh_i] == file) {
            filehandles[fh_i] = NULL;
        }
    }
    filehandle_mutex->unlock();
}
}

extern "C" uint32_t  __HeapLimit;

// Linker defined symbol used by _sbrk to indicate where heap should start.
extern "C" uint32_t __end__;
// Weak attribute allows user to override, e.g. to use external RAM for dynamic memory.
extern "C" WEAK caddr_t _sbrk(int incr)
{
    static unsigned char *heap = (unsigned char *)&__end__;
    unsigned char        *prev_heap = heap;
    unsigned char        *new_heap = heap + incr;

    if (new_heap >= (unsigned char *)&__HeapLimit) {    /* __HeapLimit is end of heap section */
        errno = ENOMEM;
        return (caddr_t) -1;
    }

    // Additional heap checking if set
    if (mbed_heap_size && (new_heap >= mbed_heap_start + mbed_heap_size)) {
        errno = ENOMEM;
        return (caddr_t) -1;
    }

    heap = new_heap;
    return (caddr_t) prev_heap;
}

// Stub out locks when an rtos is not present
extern "C" WEAK void __rtos_malloc_lock(struct _reent *_r) {}
extern "C" WEAK void __rtos_malloc_unlock(struct _reent *_r) {}
extern "C" WEAK void __rtos_env_lock(struct _reent *_r) {}
extern "C" WEAK void __rtos_env_unlock(struct _reent *_r) {}

extern "C" void __malloc_lock(struct _reent *_r)
{
    __rtos_malloc_lock(_r);
}

extern "C" void __malloc_unlock(struct _reent *_r)
{
    __rtos_malloc_unlock(_r);
}

extern "C" void __env_lock(struct _reent *_r)
{
    __rtos_env_lock(_r);
}

extern "C" void __env_unlock(struct _reent *_r)
{
    __rtos_env_unlock(_r);
}

extern "C" void __real_exit(int return_code);

extern "C" void __wrap_exit(int return_code)
{
    __real_exit(return_code);
}

extern "C" int __wrap_atexit(void (*func)())
{
    return 1;
}
