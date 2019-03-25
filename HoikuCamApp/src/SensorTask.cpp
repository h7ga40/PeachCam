#include <limits.h>
#include "mbed.h"
#include "SensorTask.h"
#include "GlobalState.h"

extern uint8_t heart_mark;

TriggerButtonTask::TriggerButtonTask(SensorTask *owner) :
	Task(10),
	_owner(owner),
	button(D6),
	_state((button != 0) ? State::Down : State::Up),
	_count(0)
{
}

TriggerButtonTask::~TriggerButtonTask()
{
}

void TriggerButtonTask::Process()
{
	if (_timer != 0)
		return;

	_timer = 10;

	State::T now = (button != 0) ? State::Down : State::Up;
	if (now != _state) {
		_count++;
		if (_count > 10) {
			_state = now;
			_count = 0;

			if (now == State::Down)
				printf("Button::Down\r\n");
			else
				printf("Button::Up\r\n");

			if (now == State::Down)
				_owner->TriggerOn();
		}
	}
	else {
		_count = 0;
	}
}

GripButtonTask::GripButtonTask(SensorTask *owner) :
	Task(10),
	_owner(owner),
	button(A0),
	_state(State::Release),
	_count(0),
	_threshold(0.8f)
{
}

GripButtonTask::~GripButtonTask()
{
}

void GripButtonTask::Process()
{
	if (_timer != 0)
		return;

	_timer = 10;

	State::T now = (button < _threshold) ? State::Hold : State::Release;
	if (now != _state) {
		_count++;
		if (_count > 10) {
			_state = now;
			_count = 0;

			if (now == State::Hold)
				printf("Grip::Hold\r\n");
			else
				printf("Grip::Release\r\n");
		}
	}
	else {
		_count = 0;
	}
}

PowerOffTask::PowerOffTask(SensorTask *owner) :
	Task(100),
	_owner(owner),
	_state(State::PowerOff),
	_count(0)
{
}

PowerOffTask::~PowerOffTask()
{
}

void PowerOffTask::Process()
{
	if (_timer != 0)
		return;

	_timer = 100;

	switch (_state) {
	case State::PowerOff:
		if (_owner->IsGlobalActive()) {
			_state = State::PowerOn;
			_owner->PowerOn();
		}
		break;
	case State::PowerOn:
		if (!_owner->IsGlobalActive()) {
			_count++;
			if (_count > 30) {
				_count = 0;

				_owner->PowerOff();

				_state = State::PowerOff;
			}
		}
		else {
			_count = 0;
		}
		break;
	}
}

HeartRateTask::HeartRateTask(SensorTask *owner) :
	Task(osWaitForever),
	_owner(owner),
	bh1792(I2C_SDA, I2C_SCL),
	intr(A0),
	_state(State::PowerOff),
	_pos(0),
	_count(0),
	_sum(0)
{
}

HeartRateTask::~HeartRateTask()
{
}

void HeartRateTask::OnStart()
{
	int32_t ret = 0;

	// BH1792
	ret = bh1792.Init();
	if (ret < 0) {
		printf("error bh1792_Init %ld\n", ret);
		_state = State::InitError;
		_timer = osWaitForever;
		return;
	}

	intr.rise(callback(this, intr_isr));
}

void HeartRateTask::intr_isr(HeartRateTask *obj)
{
	obj->_owner->Signal(InterTaskSignals::HeartRateInt);
}

void HeartRateTask::ProcessEvent(InterTaskSignals::T signals)
{
	bh1792_prm_t prm;
	int32_t ret = 0;

	if ((signals & InterTaskSignals::PowerOn) != 0) {
		if ((_state == State::PowerOff)
			|| (_state = State::DeviceError)) {
			memset(_heart_wave, 0, sizeof(_heart_wave));
			_pos = 0;
			_count = 0;

			prm.sel_adc = BH1792_PRM_SEL_ADC_GREEN;
			//prm.msr = BH1792_PRM_MSR_SINGLE;
			prm.msr = BH1792_PRM_MSR_1024HZ;
			prm.led_en = (BH1792_PRM_LED_EN1_0 << 1) | BH1792_PRM_LED_EN2_0;
			prm.led_cur1 = BH1792_PRM_LED_CUR1_MA(1);
			prm.led_cur2 = BH1792_PRM_LED_CUR2_MA(0);
			prm.ir_th = 0xFFFC;
			//prm.int_sel = BH1792_PRM_INT_SEL_SGL;
			prm.int_sel = BH1792_PRM_INT_SEL_WTM;

			ret = bh1792.SetParams(prm);
			if (ret < 0) {
				printf("error bh1792_SetParams %ld\n", ret);
				_state = State::DeviceError;
				_timer = osWaitForever;
				return;
			}

			ret = bh1792.StartMeasure();
			if (ret < 0) {
				printf("error bh1792_StartMeasure %ld\n", ret);
				_state = State::DeviceError;
				_timer = osWaitForever;
				return;
			}

			_state = State::Async;
			_timer = 0;
		}
	}
	if ((signals & InterTaskSignals::HeartRateInt) != 0) {
		bh1792_data_t data;
		int count, val = 0, min = UINT16_MAX, max = 0;

		if (bh1792.GetMeasData(&data) == BH1792_SUCCESS) {
			for (int i = 0; i < data.fifo_lev; i++) {
				_sum -= _heart_wave[_pos];
				uint16_t fifo_on = data.fifo[i].on;
				_heart_wave[_pos] = fifo_on;
				_sum += fifo_on;
				val += fifo_on;

				_pos++;
				if (_pos >= sizeof(_heart_wave) / sizeof(_heart_wave[0])) {
					_pos = 0;
				}
			}
			count = _count + data.fifo_lev;
			if (count > sizeof(_heart_wave) / sizeof(_heart_wave[0])) {
				count = sizeof(_heart_wave) / sizeof(_heart_wave[0]);
			}
			_count = count;

			for (int i = 0; i < count; i++) {
				uint16_t v = _heart_wave[i];
				if (min > v) min = v;
				if (max < v) max = v;
			}
			//val = (val / (int)data.fifo_lev) - ((int)_sum / (int)_count);
			val = ((2 * 0xF * val) / (int)data.fifo_lev) / ((int)max - (int)min);
			//printf("heart_mark %d < %d < %d\r\n", min, val, max);
			heart_mark = (uint8_t)((val / 2) + (val % 2));
		}
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		if (_state != State::PowerOff) {
			ret = bh1792.StopMeasure();
			if (ret < 0) {
				printf("error bh1792_StopMeasure %ld\n", ret);
				_state = State::DeviceError;
				_timer = osWaitForever;
				return;
			}

			_state = State::PowerOff;
			_timer = osWaitForever;
		}
	}
}

void HeartRateTask::Process()
{
	int32_t ret = 0;

	if (_timer != 0)
		return;

	if ((_state == State::InitError) || (_state == State::DeviceError)) {
		_timer = osWaitForever;
		return;
	}

	if (bh1792.GetMSR() <= BH1792_PRM_MSR_1024HZ) {
		_timer = 1000;		// 1Hz timer

		ret = bh1792.SetSync();
		if (ret < 0) {
			//while (1);
		}
		else if (bh1792.GetSyncSeq() < 3) {
			if (bh1792.GetSyncSeq() == 1) {
			}
			else {
				ret = bh1792.ClearFifoData();
				if (ret < 0) {
					//while (1);
				}
			}
		}
	}
	else {
		_timer = 1000 / 32;	// 32Hz timer

		ret = bh1792.StartMeasure();
		if (ret < 0) {
			//while (1);
		}
	}
}

SensorTask::SensorTask(GlobalState *globalState) :
	TaskThread(&_task),
	_task(_tasks, sizeof(_tasks) / sizeof(_tasks[0])),
	_globalState(globalState),
	triggerButton(this),
	gripButton(this),
	powerOffTask(this),
	heartRateTask(this)
{
	_tasks[0] = &triggerButton;
	_tasks[1] = &gripButton;
	_tasks[2] = &powerOffTask;
	_tasks[3] = &heartRateTask;
}

SensorTask::~SensorTask()
{
}

bool SensorTask::IsActive()
{
	return (gripButton.GetState() != GripButtonTask::State::Release);
}

bool SensorTask::IsGlobalActive()
{
	return _globalState->IsActive();
}

void SensorTask::PowerOff()
{
	_globalState->PowerOff();
}

void SensorTask::PowerOn()
{
	_globalState->PowerOn();
}

void SensorTask::TriggerOn()
{
	_globalState->MakeFilePath();
	_globalState->TriggerOn();
}

LeptonTaskThread::LeptonTaskThread(GlobalState *globalState) :
	TaskThread(&leptonTask),
	_globalState(globalState),
	leptonTask(this)
{
}

LeptonTaskThread::~LeptonTaskThread()
{
}
