#ifndef LEPTON_H
#define LEPTON_H

#include "TaskBase.h"
#include "LEPTON_Types.h"

#define PACKET_SIZE (164)
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME (60)
#define PIXEL_PER_LINE (80)
#define IMAGE_SIZE (PIXEL_PER_LINE * PACKETS_PER_FRAME)

class TaskThread;

class LeptonTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Resets,
			Capture,
			Viewing,
		};
	};
public:
	LeptonTask(TaskThread *taskThread);
	virtual ~LeptonTask();
private:
	State::T _state;
	TaskThread *_taskThread;
	mbed::SPI _spi;
	mbed::I2C _wire;
	mbed::DigitalOut _ss;
	LEP_CAMERA_PORT_DESC_T _port;
	int resets;
	uint16_t _minValue, _maxValue;
	int _packets_per_frame;
	uint8_t _frame_packet[PACKET_SIZE];
	uint16_t _image[IMAGE_SIZE];
public:
	void OnStart() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

#endif // LEPTON_H
