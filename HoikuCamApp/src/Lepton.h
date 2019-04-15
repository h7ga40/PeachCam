#ifndef LEPTON_H
#define LEPTON_H

#include "TaskBase.h"
#include "LEPTON_Types.h"

struct TLeptonTelemetryStatusBits {
	uint16_t : 3;
	uint16_t FFC_Desired : 1;
	uint16_t FFC_State : 2;
	uint16_t : 6;
	uint16_t AGC_State : 1;
	uint16_t : 2;
	uint16_t ShutterLockout : 1;
	uint16_t : 4;
	uint16_t OvertempShutDownImminent : 1;
	uint16_t : 11;
} __attribute__((packed));

struct TLeptonROI {
	uint16_t startRow;
	uint16_t startCol;
	uint16_t endRow;
	uint16_t endCol;
} __attribute__((packed));

struct TLeptonTelemetryA {
	uint16_t Header[2];
	uint16_t TelemetryRevision;
	uint32_t TimeCounter;
	TLeptonTelemetryStatusBits StatusBits;
	uint8_t ModuleSerialNo[16];
	uint16_t SoftwareRevision[4];
	uint16_t Reserved1[3];
	uint32_t FrameCounter;
	uint16_t FrameMean;
	uint16_t FPA_Temp[2];
	uint16_t HousingTemp[2];
	uint16_t Reserved2[2];
	uint16_t FPA_TempAtLastFFC;
	uint32_t TimeCounterAtLastFFC;
	uint16_t HousingTempAtLastFFC;
	uint16_t Reserved3[1];
	TLeptonROI AGC_ROI;
	uint16_t AGC_ClipLimitHigh;
	uint16_t AGC_ClipLimitLow;
	uint16_t Reserved4[32];
	uint32_t VideoOutputFormat;
	uint16_t Log2OfFFC;
} __attribute__((packed));

struct TLeptonTelemetryB {
	uint16_t Header[2];
	uint16_t Reserved1[19];
	uint16_t Emissivity;
	uint16_t BackgroundTemperature;
	uint16_t AtmosphericTransmission;
	uint16_t AtmosphericTemperature;
	uint16_t WindowTransmission;
	uint16_t WindowReflection;
	uint16_t WindowTemperature;
	uint16_t WindowReflectedTemperature;
} __attribute__((packed));

struct TLeptonTelemetryC {
	uint16_t Header[2];
	uint16_t Reserved1[5];
	uint16_t GainMode;
	uint16_t EffectiveGainMode;
	uint16_t GainModeDesiredFlag;
	uint16_t TemperatureGainModeThresholdHighToLowCelsius;
	uint16_t TemperatureGainModeThresholdLowToHighCelsius;
	uint16_t TemperatureGainModeThresholdHighToLowKelvin;
	uint16_t TemperatureGainModeThresholdLowToHighKelvin;
	uint16_t Reserved2[2];
	uint16_t PopulationGainModeThresholdHighToLow;
	uint16_t PopulationGainModeThresholdLowToHigh;
	uint16_t Reserved3[6];
	TLeptonROI GainModeROI;
	uint16_t Reserved4[22];
	uint16_t TLinearEnableState;
	uint16_t TLinearResolution;
	uint16_t SpotmeterMean;
	uint16_t SpotmeterMaximum;
	uint16_t SpotmeterMinimum;
	uint16_t SpotmeterPopulation;
	TLeptonROI SpotmeterROI;
} __attribute__((packed));

struct lepton_config_t {
	int radiometry;
	int ffcnorm;
	int telemetry;
	int offset;
	int color;
};

#define PACKET_SIZE (164)
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME (60)
#define PIXEL_PER_LINE (80)
#define BITMAP_HEADER_SIZE (70)
#define IMAGE_SIZE (BITMAP_HEADER_SIZE/2 + PIXEL_PER_LINE * PACKETS_PER_FRAME)

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
			UpdateParam,
			Viewing,
		};
	};
public:
	LeptonTask(TaskThread *taskThread);
	virtual ~LeptonTask();
private:
	State::T _state;
	TaskThread *_taskThread;
	lepton_config_t *_config;
	mbed::SPI _spi;
	mbed::I2C _wire;
	mbed::DigitalOut _ss;
	LEP_CAMERA_PORT_DESC_T _port;
	int resets;
	uint16_t _minValue, _maxValue;
	int _packets_per_frame;
	uint8_t _frame_packet[PACKET_SIZE];
	uint16_t _image[IMAGE_SIZE];
	TLeptonTelemetryA _telemetryA;
	TLeptonTelemetryB _telemetryB;
	TLeptonTelemetryC _telemetryC;
	uint16_t _fpaTemperature;
	int _radiometryReq;
	bool _runFFCNormReq;
	int _telemetryReq;
	void EnableRadiometry(bool enable);
	void RunFFCNormalization();
	void EnableTelemetry(bool enable);
public:
	void OnStart() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
	void SetConfig(lepton_config_t *config) { _config = config; }
	void SaveImage(const char *filename);
	void ReqRadiometry(bool enable) { _radiometryReq = enable ? 2 : 1; }
	void ReqFFCNormalization() { _runFFCNormReq = 1; }
	void ReqTelemetry(bool enable) { _telemetryReq = enable ? 2 : 1; }
	uint16_t GetMinValue() { return _minValue; }
	uint16_t GetMaxValue() { return _maxValue; }
	uint16_t GetTelemetryRevision() { return _telemetryA.TelemetryRevision; }
	uint32_t GetTimeCounter() { return _telemetryA.TimeCounter; }
	uint16_t GetFPA_Temp() { return _telemetryA.FPA_Temp[1]; }
	uint16_t GetWindowTemperature() { return _telemetryB.WindowTemperature; }
	uint16_t *GetTelemetryA() { return (uint16_t *)&_telemetryA; }
	uint16_t *GetTelemetryB() { return (uint16_t *)&_telemetryB; }
	uint16_t *GetTelemetryC() { return (uint16_t *)&_telemetryC; }
	uint16_t GetFpaTemperature() { return _fpaTemperature; }
};

#endif // LEPTON_H
