
#ifndef ZXING_MAIN_H
#define ZXING_MAIN_H

#include "TaskBase.h"

class GlobalState;

class ZXingTask : public TaskThread, public ITask
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Detecting,
			Detected,
		};
	};
public:
	ZXingTask(GlobalState *globalState);
	virtual ~ZXingTask();
private:
	GlobalState *_globalState;
	State::T _state;
	int _timer;
	void (*p_callback_func)(const char *addr, int size);
public:
	void Init(void (*pfunc)(const char *addr, int size));
public:
	State::T GetState() { return _state; }
	void OnStart() override;
	void OnEnd() override;
	int GetTimer() override;
	void Progress(int elapse) override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

#endif

