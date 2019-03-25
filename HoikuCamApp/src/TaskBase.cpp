#include "mbed.h"
#include "TaskBase.h"

Task::Task()
	: _timer(osWaitForever)
{
}

Task::Task(int timer)
	: _timer(timer)
{
}

Task::~Task()
{
}

void Task::OnStart()
{
}

void Task::OnEnd()
{
}

int Task::GetTimer()
{
	return _timer;
}

void Task::Progress(int elapse)
{
	_timer -= elapse;
	if (_timer < 0)
		_timer = 0;
}

void Task::ProcessEvent(InterTaskSignals::T signals)
{
	(void)signals;
}

void Task::Process()
{
}

Tasks::Tasks(ITask **tasks, int taskCount)
	: _tasks(tasks), _taskCount(taskCount)
{
}

Tasks::~Tasks()
{
}

void Tasks::OnStart()
{
	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		task->OnStart();
	}
}

void Tasks::OnEnd()
{
	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		task->OnEnd();
	}
}

int Tasks::GetTimer()
{
	int timer = -1;

	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		auto timer2 = task->GetTimer();
		if ((timer == -1) || ((timer2 != -1) && (timer > timer2)))
			timer = timer2;
	}

	return timer;
}

void Tasks::Progress(int elapse)
{
	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		task->Progress(elapse);
	}
}

void Tasks::ProcessEvent(InterTaskSignals::T signals)
{
	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		task->ProcessEvent(signals);
	}
}

void Tasks::Process()
{
	for (int i = 0; i < _taskCount; i++) {
		auto task = _tasks[i];
		task->Process();
	}
}

TaskThread::TaskThread(ITask *task, osPriority priority,
		uint32_t stack_size, unsigned char *stack_mem, const char *name) :
	_thread(priority, stack_size, stack_mem, name),
	_task(task)
{

}

TaskThread::~TaskThread()
{
}

void TaskThread::Start()
{
	_thread.start(callback(this, &TaskThread::Main));
}

void TaskThread::OnStart()
{
	_task->OnStart();
}

void TaskThread::OnEnd()
{
	_task->OnEnd();
}

void TaskThread::Main()
{
	int timer = 0;
	osEvent evt;
	us_timestamp_t now, prev;

	OnStart();

	now = ticker_read_us(get_us_ticker_data());
	for (; ; ) {
		prev = now;
		timer = _task->GetTimer();

		evt = _thread.signal_wait(0, (uint32_t)timer);
		if ((evt.status != osEventSignal)
			&& (evt.status != osEventTimeout)
			&& (evt.status != osOK))
			break;
		if (timer == 0)
			Thread::yield();

		now = ticker_read_us(get_us_ticker_data());

		timer = (int)((now / 1000) - (prev / 1000));
		_task->Progress(timer);

		if (evt.status == osEventSignal) {
			_task->ProcessEvent((InterTaskSignals::T)evt.value.signals);
			_thread.signal_clr(evt.value.signals);
		}

		_task->Process();
	}

	OnEnd();
}

void TaskThread::Signal(InterTaskSignals::T signals)
{
	if (_thread.get_id() != 0)
		_thread.signal_set((int)signals);
}
