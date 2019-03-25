#include "cmsis_os.h"
#include "kernel.h"
#include <stdlib.h>
#include <string.h>
#include "platform/MBED_ASSERT.h"
#include "mbed_boot.h"
#include "mbed_rtos_storage.h"

#define __CLZ							(uint8_t)__builtin_clz

static osStatus_t conv_error(ER ret)
{
	switch (ret) {
	case E_CTX:
		return osErrorISR;
	case E_NOID:
		return osErrorResource;
	case E_ID:
	case E_NOEXS:
		return osErrorParameter;
	case E_TMOUT:
		return osErrorTimeout;
	default:
		return osError;
	}
}

typedef struct rtos_item_s rtos_item_t;
struct rtos_item_s {
	rtos_item_t *next;
};

typedef struct rtos_queue_s {
	rtos_item_t *_last;
	int _count;
} rtos_queue_t;

rtos_item_t* rtos_queue_first(rtos_queue_t *queue) { return queue->_last->next; }
rtos_item_t* rtos_queue_last(rtos_queue_t *queue) { return queue->_last; }
int rtos_queue_count(rtos_queue_t *queue) { return queue->_count; }

bool rtos_queue_add(rtos_queue_t *queue, rtos_item_t *result)
{
	ER ret;

	ret = loc_cpu();
	if (ret < 0)
		return false;

	if (queue->_last == NULL) {
		queue->_last = result;
		queue->_last->next = result;
	}
	else {
		result->next = queue->_last->next;
		queue->_last->next = result;
	}
	queue->_count++;

	unl_cpu();

	return true;
}

osStatus_t rtos_queue_remove(rtos_queue_t *queue, rtos_item_t *result)
{
	ER ret;

	if (queue->_last == NULL)
		return osErrorParameter;

	ret = loc_cpu();
	if (ret < 0)
		return conv_error(ret);

	rtos_item_t *_first = queue->_last->next, *prv = queue->_last, *pos = _first;
	for (int i = 0; i < queue->_count; i++, prv = pos, pos = pos->next) {
		if (pos == result) {
			if (pos == queue->_last) {
				if (pos == _first) {
					queue->_last = NULL;
				}
				else {
					prv->next = queue->_last = pos->next;
				}
			}
			else {
				prv->next = pos->next;
			}
			queue->_count--;
			break;
		}
	}

	unl_cpu();

	return osOK;
}

bool rtos_queue_contains(rtos_queue_t *queue, rtos_item_t *item)
{
	if (queue->_last == NULL)
		return false;

	rtos_item_t *_first = queue->_last->next, *pos = _first;
	for (int i = 0; i < queue->_count; i++, pos = pos->next) {
		if (pos == item)
			return true;
	}

	return false;
}

typedef struct rtos_thread_s {
	struct rtos_thread_s *next;
	bool heap;
	ID tskid;
	ID flgid;
	uint32_t stack_size;
} rtos_thread_t;

rtos_queue_t osRtxThreads;

typedef struct rtos_mutex_s {
	struct rtos_mutex_s *next;
	bool heap;
	ID mtxid;
	ID tskid;
	int count;
} rtos_mutex_t;

rtos_queue_t osRtxMutexes;

typedef struct rtos_message_pool_s {
	struct rtos_message_pool_s *next;
	bool heap;
	ID mpfid;
} rtos_message_pool_t;

rtos_queue_t osRtxMessagePools;

typedef struct rtos_message_queue_s {
	struct rtos_message_queue_s *next;
	bool heap;
	ID mpfid;
	ID dtqid;
	int msg_size;
} rtos_message_queue_t;

rtos_queue_t osRtxMessageQueues;

typedef struct rtos_timer_s {
	struct rtos_timer_s *next;
	bool heap;
	ID almid;
	ID cycid;
	osTimerFunc_t func;
	osTimerType_t type;
	void *argument;
} rtos_timer_t;

rtos_queue_t osRtxTimers;

osStatus_t error_no;

rtos_thread_t main_thread_obj;

osMutexId_t singleton_mutex_id;
rtos_mutex_t singleton_mutex_obj;

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

extern "C" void sta_ker();
extern "C" void mbed_rtos_start(void) { sta_ker(); }
extern "C" void mbed_toolchain_init();

void set_main_thread(uint32_t stack_size)
{
	rtos_thread_t *thread = &main_thread_obj;
	rtos_mutex_t *mutex = &singleton_mutex_obj;
	ER ret;
	ID tskid = 0;
	T_CMTX cmtx;
	T_CFLG cflg;

	memset(&cmtx, 0, sizeof(cmtx));

	cmtx.ceilpri = TMAX_TPRI;

	ret = acre_mtx(&cmtx);
	if (ret < 0) {
		MBED_ASSERT(false);
		return;
	}

	mutex->mtxid = ret;
	mutex->count = 0;

	singleton_mutex_id = (osMutexId_t)mutex;

	ret = get_tid(&tskid);
	if (ret < 0) {
		MBED_ASSERT(false);
		return;
	}

	thread->tskid = tskid;
	thread->stack_size = stack_size;

	memset(&cflg, 0, sizeof(cflg));

	ret = acre_flg(&cflg);
	if (ret < 0) {
		MBED_ASSERT(false);
		return;
	}

	thread->flgid = (ID)ret;
}

extern "C" void init_cmsis_os(uint32_t stack_size)
{
	rtos_thread_t *thread = &main_thread_obj;
	rtos_mutex_t *mutex = &singleton_mutex_obj;

	unsigned char *free_start = HEAP_START;
	uint32_t free_size = HEAP_SIZE;

	mbed_init();

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

	set_main_thread(stack_size);

	rtos_queue_add(&osRtxThreads, (rtos_item_t *)thread);
	rtos_queue_add(&osRtxMutexes, (rtos_item_t *)mutex);

	mbed_start();
}

extern "C" osStatus_t osDelay(uint32_t ticks)
{
	ER ret;

	ret = dly_tsk(ticks * 1000);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" uint32_t osKernelGetTickCount(void)
{
	ER ret;
	SYSTIM time;

	ret = get_tim(&time);
	if (ret < 0)
		return conv_error(ret);

	return (uint32_t)(time / 1000);
}

extern "C" osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
{
	ER_ID ret;
	T_CFLG cflg;

	memset(&cflg, 0, sizeof(cflg));

	ret = acre_flg(&cflg);
	if (ret < 0){
		error_no = conv_error(ret);
		return NULL;
	}

	return (osEventFlagsId_t)ret;
}

extern "C" uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
{
	ER ret;
	T_RFLG rflg;

	ret = set_flg((ID)ef_id, flags);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	ret = ref_flg((ID)ef_id, &rflg);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)
{
	ER ret;
	T_RFLG rflg;

	ret = ref_flg((ID)ef_id, &rflg);
	if (ret < 0)
		return conv_error(ret);

	ret = clr_flg((ID)ef_id, ~flags);
	if (ret < 0)
		return conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)
{
	ER ret;
	T_RFLG rflg;

	ret = ref_flg((ID)ef_id, &rflg);
	if (ret < 0)
		return conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout)
{
	ER ret;
	MODE mode;
	FLGPTN flgptn;

	switch (options & 1) {
	case osFlagsWaitAny:
		mode = TWF_ORW;
		break;
	case osFlagsWaitAll:
		mode = TWF_ANDW;
		break;
	default:
		mode = 0;
		break;
	}

	memset(&flgptn, 0, sizeof(flgptn));

	ret = twai_flg((ID)ef_id, flags, mode, &flgptn, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	if ((options & osFlagsNoClear) == 0) {
		ret = clr_flg((ID)ef_id, ~(flags & flgptn));
		if (ret < 0)
			return (uint32_t)conv_error(ret);
	}

	return osOK;
}

extern "C" osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)
{
	ER ret;

	ret = del_flg((ID)ef_id);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osMemoryPoolId_t osMemoryPoolNew(uint32_t block_count, uint32_t block_size, const osMemoryPoolAttr_t *attr)
{
	rtos_message_pool_t *result;
	ER_ID ret;
	T_CMPF cmpf;
	uint32_t size;

	if ((block_count == 0) || (block_size == 0)) {
		error_no = osErrorParameter;
		return NULL;
	}

	if ((attr != NULL) && (attr->cb_mem != NULL) && (attr->cb_size >= sizeof(rtos_message_pool_t))) {
		result = (rtos_message_pool_t *)attr->cb_mem;
	}
	else {
		result = new rtos_message_pool_t();
		if (result == NULL) {
			error_no = osErrorResource;
			return NULL;
		}
	}
	memset(result, 0, sizeof(*result));
	result->heap = ((void *)result != attr->cb_mem);
	rtos_queue_add(&osRtxMessagePools, (rtos_item_t *)result);

	memset(&cmpf, 0, sizeof(cmpf));

	cmpf.blkcnt = block_count;
	cmpf.blksz = block_size;

	size = block_size * block_count;

	if (attr != NULL) {
		if (attr->mp_size > size)
			cmpf.mpf = (MPF_T *)attr->mp_mem;
		if ((attr->mp_size - size) > (sizeof(intptr_t) * block_count))
			cmpf.mpfmb = &((uint8_t *)attr->mp_mem)[size];
	}
	if (cmpf.mpf == NULL){
		cmpf.mpf = (MPF_T *)malloc(size);
	}
	if (cmpf.mpfmb == NULL){
		cmpf.mpfmb = (MPF_T *)malloc(sizeof(intptr_t) * block_count);
	}

	ret = acre_mpf(&cmpf);
	if (ret < 0) {
		error_no = conv_error(ret);
		return NULL;
	}

	result->mpfid = ret;

	return (osMemoryPoolId_t)result;
}

extern "C" void *osMemoryPoolAlloc(osMemoryPoolId_t mp_id, uint32_t timeout)
{
	rtos_message_pool_t *result = (rtos_message_pool_t *)mp_id;
	ER ret;
	void *blk = NULL;

	if (!rtos_queue_contains(&osRtxMessagePools, (rtos_item_t *)result))
		return (void *)osErrorParameter;

	ret = tget_mpf(result->mpfid, &blk, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return (void *)conv_error(ret);

	return blk;
}

extern "C" osStatus_t osMemoryPoolFree(osMemoryPoolId_t mp_id, void *block)
{
	rtos_message_pool_t *result = (rtos_message_pool_t *)mp_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxMessagePools, (rtos_item_t *)result))
		return osErrorParameter;

	ret = rel_mpf(result->mpfid, block);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" uint32_t osMemoryPoolGetBlockSize(osMemoryPoolId_t mp_id)
{
	rtos_message_pool_t *result = (rtos_message_pool_t *)mp_id;
	ER ret;
	T_RMPF rmpf;

	if (!rtos_queue_contains(&osRtxMessagePools, (rtos_item_t *)result))
		return (uint32_t)osErrorParameter;

	memset(&rmpf, 0, sizeof(rmpf));
	ret = ref_mpf(result->mpfid, &rmpf);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return rmpf.fblkcnt;
}

extern "C" osStatus_t osMemoryPoolDelete(osMemoryPoolId_t mp_id)
{
	rtos_message_pool_t *result = (rtos_message_pool_t *)mp_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxMessagePools, (rtos_item_t *)result))
		return osErrorParameter;

	ret = del_mpf(result->mpfid);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
	rtos_message_queue_t *result;
	uint32_t block_size;
	uint32_t size;
	ER_ID ret;
	T_CDTQ cdtq;
	T_CMPF cmpf;

	if ((msg_count == 0) || (msg_size == 0)) {
		error_no = osErrorParameter;
		return NULL;
	}

	block_size = ((msg_size + 3U) & ~3UL) + sizeof(osRtxMessage_t);
	if ((__CLZ(msg_count) + __CLZ(block_size)) < 32U) {
		error_no = osErrorParameter;
		return NULL;
	}

	if ((attr != NULL) && (attr->cb_mem != NULL) && (attr->cb_size >= sizeof(rtos_message_queue_t))) {
		result = (rtos_message_queue_t *)attr->cb_mem;
	}
	else {
		result = new rtos_message_queue_t();
		if (result == NULL) {
			error_no = osErrorResource;
			return NULL;
		}
	}
	memset(result, 0, sizeof(*result));
	result->heap = ((void *)result != attr->cb_mem);
	rtos_queue_add(&osRtxMessageQueues, (rtos_item_t *)result);

	memset(&cdtq, 0, sizeof(cdtq));
	memset(&cmpf, 0, sizeof(cmpf));

	cdtq.dtqcnt = msg_count;
	cdtq.dtqmb = malloc(sizeof(intptr_t) * msg_count);

	ret = acre_dtq(&cdtq);
	if (ret < 0) {
		rtos_queue_remove(&osRtxMessageQueues, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	result->dtqid = ret;

	cmpf.blkcnt = msg_count;
	cmpf.blksz = block_size;

	size = block_size * msg_count;

	if (attr != NULL) {
		if (attr->mq_size > size)
			cmpf.mpf = (MPF_T *)attr->mq_mem;
		if ((attr->mq_size - size) > (sizeof(intptr_t) * msg_count))
			cmpf.mpfmb = &((uint8_t *)attr->mq_mem)[size];
	}
	if (cmpf.mpf == NULL){
		cmpf.mpf = (MPF_T *)malloc(size);
	}
	if (cmpf.mpfmb == NULL){
		cmpf.mpfmb = (MPF_T *)malloc(sizeof(intptr_t) * msg_count);
	}

	ret = acre_mpf(&cmpf);
	if (ret < 0) {
		del_dtq(result->dtqid);
		rtos_queue_remove(&osRtxMessageQueues, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	result->mpfid = ret;
	result->msg_size = msg_size;

	return (osMessageQueueId_t)result;
}

extern "C" osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
	rtos_message_queue_t *result = (rtos_message_queue_t *)mq_id;
	ER ret;
	osRtxMessage_t *msg = NULL;

	if (!rtos_queue_contains(&osRtxMessageQueues, (rtos_item_t *)result))
		return osErrorParameter;

	ret = tget_mpf(result->mpfid, (void **)&msg, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return conv_error(ret);

	memcpy(&msg[1], msg_ptr, result->msg_size);
	msg->id = osRtxIdMessage;
	msg->flags = 0U;
	msg->priority = msg_prio;

	ret = tsnd_dtq(result->dtqid, (intptr_t)msg, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0) {
		rel_mpf(result->mpfid, msg);
		return conv_error(ret);
	}

	return osOK;
}

extern "C" osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
	rtos_message_queue_t *result = (rtos_message_queue_t *)mq_id;
	ER ret;
	osRtxMessage_t *msg = NULL;

	if (!rtos_queue_contains(&osRtxMessageQueues, (rtos_item_t *)result))
		return osErrorParameter;

	ret = trcv_dtq(result->dtqid, (intptr_t *)&msg, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return conv_error(ret);

	memcpy(msg_ptr, &msg[1], result->msg_size);
	if (msg_prio != NULL)
		*msg_prio = (uint8_t)msg->priority;
	msg->id = osRtxIdInvalid;

	ret = rel_mpf(result->mpfid, msg);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
{
	rtos_message_queue_t *result = (rtos_message_queue_t *)mq_id;
	ER ret1, ret2;

	if (!rtos_queue_contains(&osRtxMessageQueues, (rtos_item_t *)result))
		return osErrorParameter;

	ret1 = del_dtq(result->dtqid);
	ret2 = del_mpf(result->mpfid);

	if (ret1 < 0)
		return conv_error(ret1);

	if (ret2 < 0)
		return conv_error(ret2);

	return osOK;
}

extern "C" osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
	rtos_mutex_t *result;
	ER_ID ret;
	T_CMTX cmtx;

	if ((attr != NULL) && (attr->cb_mem != NULL) && (attr->cb_size >= sizeof(rtos_mutex_t))) {
		result = (rtos_mutex_t *)attr->cb_mem;
	}
	else {
		result = new rtos_mutex_t();
		if (result == NULL) {
			error_no = osErrorResource;
			return NULL;
		}
	}
	memset(result, 0, sizeof(*result));
	result->heap = ((void *)result != attr->cb_mem);
	rtos_queue_add(&osRtxMutexes, (rtos_item_t *)result);

	memset(&cmtx, 0, sizeof(cmtx));

	cmtx.ceilpri = TMAX_TPRI;

	ret = acre_mtx(&cmtx);
	if (ret < 0) {
		rtos_queue_remove(&osRtxMutexes, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	result->mtxid = ret;
	result->count = 0;

	return (osMutexId_t)result;
}

extern "C" osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
	rtos_mutex_t *result = (rtos_mutex_t *)mutex_id;
	ER ret;
	ID tskid = 0;

	if (!rtos_queue_contains(&osRtxMutexes, (rtos_item_t *)result))
		return osErrorParameter;

	ret = get_tid(&tskid);
	if (ret < 0)
		return conv_error(ret);

	ret = tloc_mtx(result->mtxid, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret == E_OBJ) {
		result->count++;
		return osOK;
	}
	if (ret < 0)
		return conv_error(ret);

	result->tskid = tskid;

	return osOK;
}

extern "C" osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
	rtos_mutex_t *result = (rtos_mutex_t *)mutex_id;
	ER ret;
	ID tskid = 0;

	if (!rtos_queue_contains(&osRtxMutexes, (rtos_item_t *)result))
		return osErrorParameter;

	ret = get_tid(&tskid);
	if (ret < 0)
		return conv_error(ret);

	ret = loc_cpu();
	if (ret < 0)
		return osErrorISR;

	if ((result->tskid == tskid) && (result->count > 0)) {
		result->count--;
		unl_cpu();

		return osOK;
	}

	unl_cpu();

	ret = unl_mtx(result->mtxid);
	if (ret < 0)
		return conv_error(ret);

	result->tskid = 0;

	return osOK;
}

extern "C" osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
	rtos_mutex_t *result = (rtos_mutex_t *)mutex_id;
	rtos_thread_t *thread = NULL;
	ER ret;
	T_RMTX rmtx;

	if (!rtos_queue_contains(&osRtxMutexes, (rtos_item_t *)result))
		return (osThreadId_t)osErrorParameter;

	ret = ref_mtx(result->mtxid, &rmtx);
	if (ret < 0)
		return (osThreadId_t)conv_error(ret);

	rtos_thread_t *item = (rtos_thread_t *)rtos_queue_first(&osRtxThreads);
	for (int i = 0; i < rtos_queue_count(&osRtxThreads); i++, item = item->next) {
		if (item->tskid == rmtx.htskid) {
			thread = item;
			break;
		}
	}

	return (osThreadId_t)thread;
}

extern "C" osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
	rtos_mutex_t *result = (rtos_mutex_t *)mutex_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxMutexes, (rtos_item_t *)result))
		return osErrorParameter;

	ret = del_mtx(result->mtxid);
	if (ret < 0)
		return conv_error(ret);

	rtos_queue_remove(&osRtxMutexes, (rtos_item_t *)result);
	if (result->heap)
		delete result;

	return osOK;
}

extern "C" osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
{
	ER_ID ret;
	T_CSEM csem;

	memset(&csem, 0, sizeof(csem));

	csem.maxsem = max_count;
	csem.isemcnt = initial_count;

	ret = acre_sem(&csem);
	if (ret < 0){
		error_no = conv_error(ret);
		return NULL;
	}

	return (osSemaphoreId_t)ret;
}

extern "C" osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
{
	ER ret;

	ret = twai_sem((ID)semaphore_id, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
{
	ER ret;

	ret = sig_sem((ID)semaphore_id);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)
{
	ER ret;
	T_RSEM rsem;

	ret = ref_sem((ID)semaphore_id, &rsem);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return (uint32_t)rsem.semcnt;
}

extern "C" osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)
{
	ER ret;

	ret = del_sem((ID)semaphore_id);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
	rtos_thread_t *result;
	ER_ID ret;
	T_CTSK ctsk;
	T_CFLG cflg;

	if ((attr != NULL) && (attr->cb_mem != NULL) && (attr->cb_size >= sizeof(rtos_thread_t))) 		{
		result = (rtos_thread_t *)attr->cb_mem;
	}
	else {
		result = new rtos_thread_t();
		if (result == NULL) {
			error_no = osErrorResource;
			return NULL;
		}
	}
	memset(result, 0, sizeof(*result));
	result->heap = ((void *)result != attr->cb_mem);
	rtos_queue_add(&osRtxThreads, (rtos_item_t *)result);

	memset(&ctsk, 0, sizeof(ctsk));

	ctsk.tskatr = TA_FPU;
	ctsk.task = (TASK)func;
	ctsk.exinf = (intptr_t)argument;

	if (attr != NULL) {
		ctsk.itskpri = osPriorityRealtime - attr->priority + TMIN_TPRI;
		ctsk.stk = (STK_T *)attr->stack_mem;
		ctsk.stksz = attr->stack_size;
	}
	else {
		ctsk.stk = (STK_T *)malloc(1024);
		ctsk.stksz = 1024;
	}
	ctsk.stk = (STK_T *)((uintptr_t)ctsk.stk & ~0x7);
	ctsk.stksz = ctsk.stksz - ((uint32_t)ctsk.stk & 0x7);

	ret = acre_tsk(&ctsk);
	if (ret < 0) {
		rtos_queue_remove(&osRtxThreads, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	result->tskid = (ID)ret;
	result->stack_size = ctsk.stksz;

	memset(&cflg, 0, sizeof(cflg));

	ret = acre_flg(&cflg);
	if (ret < 0) {
		del_tsk(result->tskid);
		rtos_queue_remove(&osRtxThreads, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	result->flgid = (ID)ret;

	ret = act_tsk(result->tskid);
	if (ret < 0) {
		del_tsk(result->tskid);
		del_flg(result->flgid);
		rtos_queue_remove(&osRtxThreads, (rtos_item_t *)result);
		if (result->heap)
			delete result;
		error_no = conv_error(ret);
		return NULL;
	}

	return (osThreadId_t)result;
}

extern "C" osThreadId_t osThreadGetId(void)
{
	rtos_thread_t *result = NULL;
	ER ret;
	ID tskid = 0;

	ret = get_tid(&tskid);
	if (ret < 0)
		return (osThreadId_t)conv_error(ret);

	rtos_thread_t *item = (rtos_thread_t *)rtos_queue_first(&osRtxThreads);
	for (int i = 0; i < rtos_queue_count(&osRtxThreads); i++, item = item->next) {
		if (item->tskid == tskid) {
			result = item;
			break;
		}
	}

	return (osThreadId_t)result;
}

extern "C" uint32_t osThreadGetStackSize(osThreadId_t thread_id)
{
	rtos_thread_t *result = (rtos_thread_t *)thread_id;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (uint32_t)osErrorParameter;

	return result->stack_size;
}

extern "C" osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
{
	rtos_thread_t *result = (rtos_thread_t *)thread_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return osErrorParameter;

	ret = chg_pri(result->tskid, (PRI)(osPriorityRealtime - priority + TMIN_TPRI));
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osPriority_t osThreadGetPriority(osThreadId_t thread_id)
{
	rtos_thread_t *result = (rtos_thread_t *)thread_id;
	ER ret;
	PRI pri;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (osPriority_t)osErrorParameter;

	ret = get_pri(result->tskid, &pri);
	if (ret < 0)
		return (osPriority_t)conv_error(ret);

	return (osPriority_t)(pri - TMIN_TPRI - osPriorityRealtime);
}

extern "C" uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)
{
	rtos_thread_t *result = (rtos_thread_t *)thread_id;
	ER ret;
	T_RFLG rflg;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (uint32_t)osErrorParameter;

	ret = set_flg(result->flgid, flags);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	ret = ref_flg(result->flgid, &rflg);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osThreadFlagsClear(uint32_t flags)
{
	rtos_thread_t *result = (rtos_thread_t *)osThreadGetId();
	ER ret;
	T_RFLG rflg;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (uint32_t)osError;

	ret = ref_flg(result->flgid, &rflg);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	ret = clr_flg(result->flgid, ~flags);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osThreadFlagsGet(void)
{
	rtos_thread_t *result = (rtos_thread_t *)osThreadGetId();
	ER ret;
	T_RFLG rflg;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (uint32_t)osError;

	ret = ref_flg(result->flgid, &rflg);
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	return rflg.flgptn;
}

extern "C" uint32_t osThreadFlagsWait(uint32_t flags, uint32_t options, uint32_t timeout)
{
	rtos_thread_t *result = (rtos_thread_t *)osThreadGetId();
	ER ret;
	MODE mode;
	FLGPTN flgptn;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return (uint32_t)osError;

	switch (options & 1) {
	case osFlagsWaitAny:
		mode = TWF_ORW;
		break;
	case osFlagsWaitAll:
		mode = TWF_ANDW;
		break;
	default:
		mode = 0;
		break;
	}

	memset(&flgptn, 0, sizeof(flgptn));

	ret = twai_flg(result->flgid, flags, mode, &flgptn, (timeout == osWaitForever) ? TMO_FEVR : (timeout * 1000));
	if (ret < 0)
		return (uint32_t)conv_error(ret);

	if ((options & osFlagsNoClear) == 0) {
		ret = clr_flg(result->flgid, ~(flags & flgptn));
		if (ret < 0)
			return (uint32_t)conv_error(ret);
	}

	return flgptn;
}

extern "C" osStatus_t osThreadYield(void)
{
	ER ret;

	ret = rot_rdq(TPRI_SELF);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osStatus_t osThreadTerminate(osThreadId_t thread_id)
{
	rtos_thread_t *result = (rtos_thread_t *)thread_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxThreads, (rtos_item_t *)result))
		return osErrorParameter;

	ret = ter_tsk(result->tskid);
	if (ret < 0)
		return conv_error(ret);

	return osOK;
}

extern "C" osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
	rtos_timer_t *result;

	if ((func == NULL) || ((type != osTimerOnce) && (type != osTimerPeriodic))){
		error_no = osErrorParameter;
		return NULL;
	}

	if ((attr != NULL) && (attr->cb_mem != NULL) && (attr->cb_size >= sizeof(rtos_timer_t))) {
		result = (rtos_timer_t *)attr->cb_mem;
	}
	else {
		result = new rtos_timer_t();
		if (result == NULL) {
			error_no = osErrorResource;
			return NULL;
		}
	}
	memset(result, 0, sizeof(*result));
	result->heap = ((void *)result != attr->cb_mem);
	rtos_queue_add(&osRtxTimers, (rtos_item_t *)result);

	result->func = func;
	result->type = type;
	result->argument = argument;

	return (osTimerId_t)result;
}

extern "C" osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
{
	rtos_timer_t *result = (rtos_timer_t *)timer_id;
	ER ret;

	if (!rtos_queue_contains(&osRtxTimers, (rtos_item_t *)result))
		return osError;

	switch (result->type) {
	case osTimerOnce:
	{
		if (result->almid != 0) {
			T_CALM calm;

			memset(&calm, 0, sizeof(calm));

			calm.nfyinfo.nfymode = TNFY_HANDLER;
			calm.nfyinfo.nfy.handler.exinf = (intptr_t)result->argument;
			calm.nfyinfo.nfy.handler.tmehdr = (TMEHDR)result->func;

			ret = acre_alm(&calm);
			if (ret < 0)
				return conv_error(ret);

			result->almid = ret;
		}

		ret = sta_alm(result->almid, ticks);
		if (ret < 0)
			return conv_error(ret);
		break;
	}
	case osTimerPeriodic:
	{
		if (result->cycid != 0) {
			T_CCYC ccyc;

			memset(&ccyc, 0, sizeof(ccyc));

			ccyc.nfyinfo.nfymode = TNFY_HANDLER;
			ccyc.nfyinfo.nfy.handler.exinf = (intptr_t)result->argument;
			ccyc.nfyinfo.nfy.handler.tmehdr = (TMEHDR)result->func;
			ccyc.cyctim = ticks;

			ret = acre_cyc(&ccyc);
			if (ret < 0)
				return conv_error(ret);

			result->cycid = ret;
		}

		ret = sta_cyc(result->cycid);
		if (ret < 0)
			return conv_error(ret);
		break;
	}
	default:
		return osError;
	}

	return osOK;
}

extern "C" void rtos_attach_idle_hook(void (*fptr)(void))
{
}

extern "C" void rtos_attach_thread_terminate_hook(void (*fptr)(osThreadId_t id))
{
}
