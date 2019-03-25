
#ifndef _SENSORTASK_H_
#define _SENSORTASK_H_

#include "TaskBase.h"
#include "bh1792.h"
#include "Lepton.h"

class SensorTask;

class TriggerButtonTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			Up,
			Down
		};
	};
public:
	TriggerButtonTask(SensorTask *owner);
	virtual ~TriggerButtonTask();
private:
	SensorTask *_owner;
	mbed::DigitalIn button;
	State::T _state;
	int _count;
public:
	State::T GetState() { return _state; }
	void Process() override;
};

class GripButtonTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			Release,
			Hold
		};
	};
public:
	GripButtonTask(SensorTask *owner);
	virtual ~GripButtonTask();
private:
	SensorTask *_owner;
	mbed::AnalogIn button;
	State::T _state;
	int _count;
	float _threshold;
public:
	State::T GetState() { return _state; }
	void Process() override;
};

class PowerOffTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			PowerOn
		};
	};
public:
	PowerOffTask(SensorTask *owner);
	virtual ~PowerOffTask();
private:
	SensorTask *_owner;
	State::T _state;
	int _count;
public:
	State::T GetState() { return _state; }
	void Process() override;
};

class HeartRateTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Async,
			Sync,
			DeviceError,
			InitError,
		};
	};
public:
	HeartRateTask(SensorTask *owner);
	virtual ~HeartRateTask();
private:
	SensorTask *_owner;
	BH1792 bh1792;
	mbed::InterruptIn intr;
	State::T _state;
	uint16_t _heart_wave[2048];
	int _pos;
	int _count;
	uint32_t _sum;
	static void intr_isr(HeartRateTask *obj);
public:
	State::T GetState() { return _state; }
	void OnStart() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

class GlobalState;

class SensorTask : public TaskThread
{
public:
	SensorTask(GlobalState *globalState);
	virtual ~SensorTask();
private:
	Tasks _task;
	ITask *_tasks[4];
	GlobalState *_globalState;
	TriggerButtonTask triggerButton;
	GripButtonTask gripButton;
	PowerOffTask powerOffTask;
	HeartRateTask heartRateTask;
public:
	bool IsActive();
	bool IsGlobalActive();
	void PowerOff();
	void PowerOn();
	void TriggerOn();
};

class LeptonTaskThread : public TaskThread
{
public:
	LeptonTaskThread(GlobalState *globalState);
	virtual ~LeptonTaskThread();
private:
	GlobalState *_globalState;
	LeptonTask leptonTask;
public:
};

#endif // _SENSORTASK_H_
