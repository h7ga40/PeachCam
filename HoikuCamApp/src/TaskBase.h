
#ifndef _TASKBASE_H_
#define _TASKBASE_H_

class InterTaskSignals
{
public:
	enum T {
		WifiStatusChanged = 0x0001,
		WifiConnected = 0x0002,
		PowerOn = 0x0004,
		TriggerOn = 0x0008,
		RecAudio = 0x0010,
		UpdateRequest = 0x0020,
		UploadRequest = 0x0040,
		PowerOff = 0x0080,
		HeartRateInt = 0x0100,
		StartShutter = 0x0200,
		EndShutter = 0x0400,
	};
};

class ITask {
public:
	virtual void OnStart() = 0;
	virtual void OnEnd() = 0;
	virtual int GetTimer() = 0;
	virtual void Progress(int elapse) = 0;
	virtual void ProcessEvent(InterTaskSignals::T signals) = 0;
	virtual void Process() = 0;
};

class Task : public ITask {
public:
	Task();
	Task(int timer);
	virtual ~Task();
protected:
	int _timer;
public:
	virtual void OnStart();
	virtual void OnEnd();
	virtual int GetTimer();
	virtual void Progress(int elapse);
	virtual void ProcessEvent(InterTaskSignals::T signals);
	virtual void Process();
};

class Tasks : public ITask {
public:
	Tasks(ITask **tasks, int taskCount);
	virtual ~Tasks();
private:
	ITask **_tasks;
	int _taskCount;
public:
	virtual void OnStart();
	virtual void OnEnd();
	virtual int GetTimer();
	virtual void Progress(int elapse);
	virtual void ProcessEvent(InterTaskSignals::T signals);
	virtual void Process();
};

class TaskThread {
public:
	TaskThread(ITask *task, osPriority priority=osPriorityNormal,
		uint32_t stack_size=OS_STACK_SIZE,
		unsigned char *stack_mem=NULL, const char *name=NULL);
	virtual ~TaskThread();
protected:
	rtos::Thread _thread;
	ITask *_task;
	virtual void Main();
	static void sMain(void *obj) { ((TaskThread *)obj)->Main(); }
	virtual void OnStart();
	virtual void OnEnd();
public:
	virtual void Start();
	void Signal(InterTaskSignals::T signals);
};

#endif // _TASKBASE_H_
