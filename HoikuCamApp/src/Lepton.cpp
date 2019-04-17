#include <mbed.h>
#include <string>
#include "Lepton.h"
#include "LEPTON_SDK.h"
#include "LEPTON_SYS.h"
#include "LEPTON_OEM.h"
#include "LEPTON_RAD.h"
#include "Palettes.h"
#include "EasyAttach_CameraAndLCD.h"

#define RESULT_BUFFER_BYTE_PER_PIXEL  (2u)
#define RESULT_BUFFER_STRIDE          (((LCD_PIXEL_WIDTH * RESULT_BUFFER_BYTE_PER_PIXEL) + 31u) & ~31u)
#define RESULT_BUFFER_HEIGHT          (LCD_PIXEL_HEIGHT)
extern uint8_t user_frame_buffer_result[RESULT_BUFFER_STRIDE * RESULT_BUFFER_HEIGHT]__attribute((section("NC_BSS"), aligned(32)));

const unsigned char BMPHeader[BITMAP_HEADER_SIZE] = {
	0x42, 0x4D,					// "BM"
	0xC6, 0x25, 0x00, 0x00,		// ファイルサイズ[byte]
	0x00, 0x00,					// 予約領域１
	0x00, 0x00,					// 予約領域２
	0x46, 0x00, 0x00, 0x00,		// ファイル先頭から画像データまでのオフセット[byte]
	0x38, 0x00, 0x00, 0x00,		// 情報ヘッダサイズ[byte]
	0x50, 0x00, 0x00, 0x00,		// 画像の幅[ピクセル]
	0x3C, 0x00, 0x00, 0x00,		// 画像の高さ[ピクセル]
	0x01, 0x00,					// プレーン数
	0x10, 0x00,					// 色ビット数[bit]
	0x03, 0x00, 0x00, 0x00,		// 圧縮形式
	0x80, 0x25, 0x00, 0x00,		// 画像データサイズ[byte]
	0x13, 0x0B, 0x00, 0x00,		// 水平解像度[dot/m]
	0x13, 0x0B, 0x00, 0x00,		// 垂直解像度[dot/m]
	0x00, 0x00, 0x00, 0x00,		// 格納パレット数[使用色数]
	0x00, 0x00, 0x00, 0x00,		// 重要色数
	0x00, 0xF8, 0x00, 0x00,		// 赤成分のカラーマスク
	0xE0, 0x07, 0x00, 0x00,		// 緑成分のカラーマスク
	0x1F, 0x00, 0x00, 0x00,		// 青成分のカラーマスク
	0x00, 0x00, 0x00, 0x00		// α成分のカラーマスク
};

LeptonTask::LeptonTask(TaskThread *taskThread) :
	Task(osWaitForever),
	_state(State::PowerOff),
	_taskThread(taskThread),
	_spi(P4_6, P4_7, P4_4, NC),
	_wire(I2C_SDA, I2C_SCL),
	_ss(P4_5),
	resets(0),
	_minValue(65535), _maxValue(0),
	_packets_per_frame(60),
	_frame_packet(),
	_telemetryA(),
	_telemetryB(),
	_telemetryC(),
	_fpaTemperature(0),
	_auxTemperature(0),
	_radiometryReq(0),
	_runFFCNormReq(0),
	_telemetryReq(0),
	_spotmeterReq(0),
	_spotmeterRoi()
{
	memcpy(_image, BMPHeader, BITMAP_HEADER_SIZE);
}

LeptonTask::~LeptonTask()
{
}

const char *GetLeptonErrorString(LEP_RESULT result)
{
	switch (result) {
	case LEP_ERROR:
		return "Camera general error";
	case LEP_NOT_READY:
		return "Camera not ready error";
	case LEP_RANGE_ERROR:
		return "Camera range error";
	case LEP_CHECKSUM_ERROR:
		return "Camera checksum error";
	case LEP_BAD_ARG_POINTER_ERROR:
		return "Camera Bad argument  error";
	case LEP_DATA_SIZE_ERROR:
		return "Camera byte count error";
	case LEP_UNDEFINED_FUNCTION_ERROR:
		return "Camera undefined function error";
	case LEP_FUNCTION_NOT_SUPPORTED:
		return "Camera function not yet supported error";
	case LEP_DATA_OUT_OF_RANGE_ERROR:
		return "Camera input DATA is out of valid range error";
	case LEP_COMMAND_NOT_ALLOWED:
		return "Camera unable to execute command due to current camera state";
	/* OTP access errors */
	case LEP_OTP_WRITE_ERROR:
		return "Camera OTP write error";
	case LEP_OTP_READ_ERROR:
		return "double bit error detected (uncorrectible)";
	case LEP_OTP_NOT_PROGRAMMED_ERROR:
		return "Flag read as non-zero";
	/* I2C Errors */
	case LEP_ERROR_I2C_BUS_NOT_READY:
		return "I2C Bus Error - Bus Not Avaialble";
	case LEP_ERROR_I2C_BUFFER_OVERFLOW:
		return "I2C Bus Error - Buffer Overflow";
	case LEP_ERROR_I2C_ARBITRATION_LOST:
		return "I2C Bus Error - Bus Arbitration Lost";
	case LEP_ERROR_I2C_BUS_ERROR:
		return "I2C Bus Error - General Bus Error";
	case LEP_ERROR_I2C_NACK_RECEIVED:
		return "I2C Bus Error - NACK Received";
	case LEP_ERROR_I2C_FAIL:
		return "I2C Bus Error - General Failure";
	/* Processing Errors */
	case LEP_DIV_ZERO_ERROR:
		return "Attempted div by zero";
	/* Comm Errors */
	case LEP_COMM_PORT_NOT_OPEN:
		return "Comm port not open";
	case LEP_COMM_INVALID_PORT_ERROR:
		return "Comm port no such port error";
	case LEP_COMM_RANGE_ERROR:
		return "Comm port range error";
	case LEP_ERROR_CREATING_COMM:
		return "Error creating comm";
	case LEP_ERROR_STARTING_COMM:
		return "Error starting comm";
	case LEP_ERROR_CLOSING_COMM:
		return "Error closing comm";
	case LEP_COMM_CHECKSUM_ERROR:
		return "Comm checksum error";
	case LEP_COMM_NO_DEV:
		return "No comm device";
	case LEP_TIMEOUT_ERROR:
		return "Comm timeout error";
	case LEP_COMM_ERROR_WRITING_COMM:
		return "Error writing comm";
	case LEP_COMM_ERROR_READING_COMM:
		return "Error reading comm";
	case LEP_COMM_COUNT_ERROR:
		return "Comm byte count error";
	/* Other Errors */
	case LEP_OPERATION_CANCELED:
		return "Camera operation canceled";
	case LEP_UNDEFINED_ERROR_CODE:
		return "Undefined error";
	default:
		return "Unknown error";
	}
}

void LeptonTask::OnStart()
{
	_spi.format(8, 3/*?*/);
	//_spi.frequency(20000000);
	_spi.frequency(16000000);

	_ss = 1;
	_ss = 0;
	_ss = 1;

	ThisThread::sleep_for(185);

	printf("beginTransmission\n");

	LEP_RESULT ret = LEP_OpenPort(&_wire, LEP_CCI_TWI, 400, &_port);
	if (ret != LEP_OK) {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
		return;
	}

	LEP_SDK_VERSION_T version;
	printf("SDK version");
	ret = LEP_GetSDKVersion(&_port, &version);
	if (ret == LEP_OK) {
		printf(" %u.%u.%u\n", version.major, version.minor, version.build);
	}
	else {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}

	LEP_SYS_CUST_SERIAL_NUMBER_T cutSerialNumber;
	printf("SYS Customer Serial Number");
	ret = LEP_GetSysCustSerialNumber(&_port, &cutSerialNumber);
	if (ret == LEP_OK) {
		for (int i = 0; i < LEP_SYS_MAX_SERIAL_NUMBER_CHAR_SIZE; i++)
			printf(" %02x", *(uint8_t *)&cutSerialNumber.value[i]);
		printf("\n");
	}
	else {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}

	LEP_SYS_FLIR_SERIAL_NUMBER_T flirSerialNumber;
	printf("SYS FLiR Serial Number");
	ret = LEP_GetSysFlirSerialNumber(&_port, &flirSerialNumber);
	if (ret == LEP_OK) {
		printf(" %llu\n", flirSerialNumber);
	}
	else {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}

	LEP_SYS_UPTIME_NUMBER_T uptime;
	ret = LEP_GetSysCameraUpTime(&_port, &uptime);
	printf("SYS camera uptime");
	if (ret == LEP_OK) {
		printf(" %lu\n", uptime);
	}
	else {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}

	LEP_OEM_SW_VERSION_T oemSoftwareVersion;
	ret = LEP_GetOemSoftwareVersion(&_port, &oemSoftwareVersion);
	if (ret == LEP_OK) {
		printf("FLiR OEM software version GPP:%u.%u.%03u DSP:%u.%u.%03u\n",
			oemSoftwareVersion.gpp_major,
			oemSoftwareVersion.gpp_minor,
			oemSoftwareVersion.gpp_build,
			oemSoftwareVersion.dsp_major,
			oemSoftwareVersion.dsp_minor,
			oemSoftwareVersion.dsp_build);
	}

	LEP_OEM_PART_NUMBER_T partNumber;
	ret = LEP_GetOemFlirPartNumber(&_port, &partNumber);
	if (ret == LEP_OK) {
		printf("FLiR OEM part number %s", partNumber.value);
		if (strcmp(partNumber.value, "500-0643-00") == 0)
			printf("50 deg (l2)\n");
		else if (strcmp(partNumber.value, "500-0659-01") == 0)
			printf("shuttered 50 deg (l2)\n");
		else if (strcmp(partNumber.value, "500-0690-00") == 0)
			printf("25 deg (l2)\n");
		else if (strcmp(partNumber.value, "500-0763-01") == 0)
			printf("shuttered 50 deg + radiometric (l2.5)\n");
		else if (strcmp(partNumber.value, "500-0726-01") == 0)
			printf("shuttered 50 deg (l3)\n");
		else
			printf("unknown\n\n");
	}

	ret = LEP_GetRadSpotmeterRoi(&_port, &_spotmeterRoi);
	if (ret == LEP_OK) {
		printf("Spotmeter ROI (%d, %d) - (%d, %d)\n", _spotmeterRoi.startCol,
			_spotmeterRoi.startRow, _spotmeterRoi.endCol, _spotmeterRoi.endRow);
	}
	else {
		printf("Get spotmeter ROI %s %d\n", GetLeptonErrorString(ret), ret);
	}

	LowPower();

	EnableRadiometry(_config->radiometry);

	if (_config->ffcnorm) {
		RunFFCNormalization();
	}

	EnableTelemetry(_config->telemetry);

	PowerOn();
}

void LeptonTask::EnableRadiometry(bool enable)
{
	LEP_RAD_RADIOMETRY_FILTER_T radRadiometryFilter;
	LEP_RESULT ret = LEP_SetRadRadometryFilter(&_port, enable ? LEP_RAD_ENABLE : LEP_RAD_DISABLE);
	if (ret != LEP_OK) {
		printf("Radiometry filter set %s %d\n", GetLeptonErrorString(ret), ret);
	}
	ret = LEP_GetRadRadometryFilter(&_port, &radRadiometryFilter);
	if (ret == LEP_OK) {
		printf("Radiometry filter %s\n", radRadiometryFilter == LEP_RAD_ENABLE ? "enabled" : "disabled");
	}
	if (enable) {
		LEP_RAD_ENABLE_E tLinear;
		ret = LEP_SetRadTLinearEnableState(&_port, LEP_RAD_ENABLE);
		if (ret != LEP_OK) {
			printf("TLinear enable %s %d\n", GetLeptonErrorString(ret), ret);
		}
		ret = LEP_GetRadTLinearEnableState(&_port, &tLinear);
		if (ret == LEP_OK) {
			printf("TLinear enable %s\n", tLinear == LEP_RAD_ENABLE ? "enabled" : "disabled");
		}
	}
}

void LeptonTask::RunFFCNormalization()
{
	printf("Flat-Field Correction normalization");
	LEP_RESULT ret = LEP_RunSysFFCNormalization(&_port);
	if (ret != LEP_OK) {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}
	else {
		printf("\n");
	}
}

void LeptonTask::EnableTelemetry(bool enable)
{
	LEP_RESULT ret;

	if (enable) {
		printf("SYS Telemetry Location");
		ret = LEP_SetSysTelemetryLocation(&_port, LEP_TELEMETRY_LOCATION_FOOTER);
		if (ret != LEP_OK) {
			printf(" %s %d\n", GetLeptonErrorString(ret), ret);
		}
		else {
			printf(" Footer\n");
		}
	}

	printf("SYS Telemetry");
	ret = LEP_SetSysTelemetryEnableState(&_port, enable ? LEP_TELEMETRY_ENABLED : LEP_TELEMETRY_DISABLED);
	if (ret != LEP_OK) {
		printf(" %s %d\n", GetLeptonErrorString(ret), ret);
	}
	else if (enable) {
		printf(" Enable\n");
	}
	else {
		printf(" Disable\n");
	}

	LEP_SYS_TELEMETRY_ENABLE_STATE_E telemetory = LEP_TELEMETRY_DISABLED;
	LEP_GetSysTelemetryEnableState(&_port, &telemetory);
	if (telemetory != 0) {
		_packets_per_frame = 63;
	}
	else {
		_packets_per_frame = 60;
	}

	printf("Packet Per Frame %d\n", _packets_per_frame);
}

void LeptonTask::GetSpotmeterObj()
{
	LEP_RESULT ret;
	LEP_RAD_SPOTMETER_OBJ_KELVIN_T spotmeterObj;

	ret = LEP_GetRadSpotmeterObjInKelvinX100(&_port, &spotmeterObj);
	if (ret == LEP_OK) {
		int temp1 = spotmeterObj.radSpotmeterMaxValue - 27315;
		int temp2 = spotmeterObj.radSpotmeterMinValue - 27315;
		printf("Spotmeter max:%d.%02u min:%d.%02u\n",
			temp1 / 100, (temp1 > 0) ? (temp1 % 100) : (100 - temp1 % 100),
			temp2 / 100, (temp2 > 0) ? (temp2 % 100) : (100 - temp2 % 100));

		temp1 = spotmeterObj.radSpotmeterPopulation - 27315;
		temp2 = spotmeterObj.radSpotmeterValue - 27315;
		printf("Spotmeter pop:%d.%02u val:%d.%02u\n",
			temp1 / 100, (temp1 > 0) ? (temp1 % 100) : (100 - temp1 % 100),
			temp2 / 100, (temp2 > 0) ? (temp2 % 100) : (100 - temp2 % 100));
	}
	else {
		printf("Get spotmeter value %s %d\n", GetLeptonErrorString(ret), ret);
	}
}

void LeptonTask::SetSpotmeterRoi(LEP_RAD_ROI_T newRoi)
{
	LEP_RESULT ret;

	ret = LEP_SetRadSpotmeterRoi(&_port, newRoi);
	if (ret == LEP_OK) {
		_spotmeterRoi = newRoi;
	}
	else {
		printf("Set spotmeter ROI %s %d\n", GetLeptonErrorString(ret), ret);
	}
}

void LeptonTask::LowPower()
{
	LEP_RESULT ret;

	ret = LEP_RunOemLowPowerMode2(&_port);
	if (ret != LEP_OK) {
		printf("Set low power mode 2 %s %d\n", GetLeptonErrorString(ret), ret);
	}
}

void LeptonTask::PowerOn()
{
	LEP_RESULT ret;

	ret = LEP_RunOemPowerOn(&_port);
	if (ret != LEP_OK) {
		printf("Set power on %s %d\n", GetLeptonErrorString(ret), ret);
	}
}

void LeptonTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::PowerOn) != 0) {
		_ss = 0;
		_state = State::Resets;
		_timer = 750;
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		_state = State::PowerOff;
		_timer = -1;
	}
}

void LeptonTask::Process()
{
	uint8_t *result = _frame_packet;
	uint16_t *frameBuffer;
	uint16_t value, minValue, maxValue;
	float diff, scale;
	int packet_id;
	uint16_t *values;

	if (_timer != 0)
		return;

	switch (_state) {
	case State::Resets:
		_ss = 1;
		_state = State::Capture;
		_timer = 1;
		break;
	case State::Capture:
		minValue = 65535;
		maxValue = 0;
		packet_id = -1;
		for (int row = 0; row < _packets_per_frame; ) {
			_spi.lock();
			_ss = 0;
			_spi.write(NULL, 0, (char *)result, PACKET_SIZE);
			_ss = 1;
			_spi.unlock();

			int id = (result[0] << 8) | result[1];
			if (id == packet_id) {
				row++;
				continue;
			}
			else if ((id & 0x0F00) == 0x0F00) {
				if (packet_id != -1) {
					row++;
					continue;
				}
				_state = State::Capture;
				_timer = 0;
				return;
			}

			if ((packet_id == -1) && ((id & 0x0FFF) != 0x7FF)) {
				int r = 2 * (id & 0x00FF);
				if (r >= _packets_per_frame)
					continue;
				row = r;
			}
			packet_id = id;

			if (row < PACKETS_PER_FRAME) {
				uint16_t *pixel = &_image[BITMAP_HEADER_SIZE / 2 + PIXEL_PER_LINE * (PACKETS_PER_FRAME - 1 - row)];
				frameBuffer = (uint16_t *)result;
				for (int i = 2; i < PACKET_SIZE_UINT16; i++) {
					value = frameBuffer[i];
					value = (value >> 8) | (value << 8);
					//frameBuffer[i] = value;

					if (value > maxValue) {
						maxValue = value;
					}
					if (value < minValue) {
						minValue = value;
					}
					*pixel++ = value;
				}
			}
			else switch (row - PACKETS_PER_FRAME) {
			case 0:
				frameBuffer = (uint16_t *)result;
				memcpy(&_telemetryA, frameBuffer, sizeof(_telemetryA));
				break;
			case 1:
				frameBuffer = (uint16_t *)result;
				memcpy(&_telemetryB, frameBuffer, sizeof(_telemetryB));
				break;
			case 2:
				frameBuffer = (uint16_t *)result;
				memcpy(&_telemetryC, frameBuffer, sizeof(_telemetryC));
				break;
			}

			row++;
		}

		if (packet_id == -1) {
			_state = State::Capture;
			_timer = 0;
			return;
		}

		_maxValue = maxValue;
		_minValue = minValue;

		resets++;
		_state = State::UpdateParam;
		_timer = 0;
		break;
	case State::UpdateParam:
		LEP_GetSysFpaTemperatureKelvin(&_port, &_fpaTemperature);
		LEP_GetSysAuxTemperatureKelvin(&_port, &_auxTemperature);
		switch (_radiometryReq) {
		case 1:
			_radiometryReq = 0;
			EnableRadiometry(false);
			break;
		case 2:
			_radiometryReq = 0;
			EnableRadiometry(true);
			break;
		}
		if (_runFFCNormReq) {
			_runFFCNormReq = 0;
			RunFFCNormalization();
		}
		switch (_telemetryReq) {
		case 1:
			_telemetryReq = 0;
			EnableTelemetry(false);
			break;
		case 2:
			_telemetryReq = 0;
			EnableTelemetry(true);
			break;
		}
		switch (_spotmeterReq) {
		case 1:
			_spotmeterReq = 0;
			GetSpotmeterObj();
			break;
		case 2:
			_spotmeterReq = 0;
			SetSpotmeterRoi(_reqSpotmeterRoi);
			break;
		}
		_state = State::Viewing;
		_timer = 0;
		break;
	case State::Viewing:
		maxValue = _maxValue;
		minValue = _minValue;
		diff = maxValue - minValue;
		if (diff < 256) {
			diff = 256;
			minValue = (maxValue + minValue) / 2 - 128;
		}
		scale = 255.9 / diff;

		values = &_image[BITMAP_HEADER_SIZE / 2];
		for (int row = 0; row < PACKETS_PER_FRAME; row++) {
			uint16_t *pixel = &((uint16_t *)&user_frame_buffer_result)[(LCD_PIXEL_WIDTH - 1 - PIXEL_PER_LINE) + (LCD_PIXEL_HEIGHT - 1 - row) * LCD_PIXEL_WIDTH];
			for (int column = 0; column < PIXEL_PER_LINE; column++) {
				uint16_t value = *values;
				uint8_t index;
				int colormap[3];

				switch (_config->color) {
				case 0:
					index = (value - minValue) * scale;
					colormap[0] = colormap_rainbow[3 * index];
					colormap[1] = colormap_rainbow[3 * index + 1];
					colormap[2] = colormap_rainbow[3 * index + 2];
					break;
				case 1:
					index = (value - minValue) * scale;
					colormap[0] = colormap_grayscale[3 * index];
					colormap[1] = colormap_grayscale[3 * index + 1];
					colormap[2] = colormap_grayscale[3 * index + 2];
					break;
				case 2:
					index = (value - minValue) * scale;
					colormap[0] = colormap_ironblack[3 * index];
					colormap[1] = colormap_ironblack[3 * index + 1];
					colormap[2] = colormap_ironblack[3 * index + 2];
					break;
				default:
				{
					int span = 256;
					int max = span - 1;
					int h = value % (max * 6);
					int s;
					int b;
					if (value < (1 << 13)) {
						s = (span * value) / (1 << 13);
						b = 0;
					}
					else {
						value -= (1 << 13);
						b = (span * value) / (1 << 13);
						s = span - b;
					}

					int p = (h / max) % 6;
					h %= span;
					switch (p) {
					case 0:
						colormap[0] = b;
						colormap[1] = ((s * h) / span) + b;
						colormap[2] = ((s * max) / span) + b;
						break;
					case 1:
						colormap[0] = b;
						colormap[1] = ((s * max) / span) + b;
						colormap[2] = ((s * (max - h)) / span) + b;
						break;
					case 2:
						colormap[0] = ((s * h) / span) + b;
						colormap[1] = ((s * max) / span) + b;
						colormap[2] = b;
						break;
					case 3:
						colormap[0] = ((s * max) / span) + b;
						colormap[1] = ((s * (max - h)) / span) + b;
						colormap[2] = b;
						break;
					case 4:
						colormap[0] = ((s * max) / span) + b;
						colormap[1] = b;
						colormap[2] = ((s * h) / span) + b;
						break;
					default:
						colormap[0] = ((s * (max - h)) / span) + b;
						colormap[1] = b;
						colormap[2] = ((s * max) / span) + b;
						break;
					}
					colormap[0] = 256 * colormap[0] / span;
					colormap[1] = 256 * colormap[1] / span;
					colormap[2] = 256 * colormap[2] / span;
					break;
				}
				}
				// ARGB4444
				*pixel++ = 0xF000 | ((colormap[0] >> 4) << 8) | ((colormap[1] >> 4) << 4) | ((colormap[2] >> 4) << 0);
				values++;
			}
		}

		// https://lepton.flir.com/application-notes/lepton-with-radiometry/
		if (resets == 750) {
			resets = 0;
			_ss = 0;
			_state = State::Resets;
			_timer = 750;
		}
		else {
			_state = State::Capture;
			_timer = 100;
		}
		break;
	default:
		_state = State::PowerOff;
		_timer = -1;
		break;
	}
}

void LeptonTask::SaveImage(const char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL)
		return;
	fwrite(_image, sizeof(uint16_t), IMAGE_SIZE, fp);
	fclose(fp);
}

extern "C" {

	LEP_RESULT LEP_I2C_MasterOpen(LEP_PORTID portID,
		LEP_UINT16 *portBaudRate)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;

		wire->frequency(*portBaudRate * 1000);

		return result;
	}

	LEP_RESULT LEP_I2C_MasterClose(LEP_CAMERA_PORT_DESC_T_PTR portDescPtr)
	{
		mbed::I2C *wire = (mbed::I2C *)portDescPtr->portID;
		LEP_RESULT result = LEP_OK;

		(void)wire;

		return result;
	}

	LEP_RESULT LEP_I2C_MasterReset(LEP_CAMERA_PORT_DESC_T_PTR portDescPtr)
	{
		mbed::I2C *wire = (mbed::I2C *)portDescPtr->portID;
		LEP_RESULT result = LEP_OK;

		(void)wire;

		return result;
	}

	LEP_RESULT LEP_I2C_MasterReadData(LEP_PORTID portID,
		LEP_UINT8  deviceAddress,
		LEP_UINT16 subAddress,
		LEP_UINT16 *dataPtr,
		LEP_UINT16 dataLength)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;
		int error;
		LEP_UINT8 *data, *pos;

		pos = data = (LEP_UINT8 *)dataPtr;
		*pos++ = (LEP_UINT8)(subAddress >> 8);
		*pos++ = (LEP_UINT8)subAddress;

		error = wire->write(deviceAddress << 1, (char *)data, sizeof(subAddress));
		if (error != 0)
			return LEP_ERROR_I2C_FAIL;

		error = wire->read(deviceAddress << 1, (char *)dataPtr, sizeof(LEP_UINT16) * dataLength);
		if (error != 0)
			return LEP_ERROR_I2C_FAIL;

		for (int i = 0; i < dataLength; i++) {
			LEP_UINT16 temp = dataPtr[i];
			dataPtr[i] = (temp >> 8) | (temp << 8);
		}

		return result;
	}

	LEP_RESULT LEP_I2C_MasterWriteData(LEP_PORTID portID,
		LEP_UINT8  deviceAddress,
		LEP_UINT16 subAddress,
		LEP_UINT16 *dataPtr,
		LEP_UINT16 dataLength)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;
		int error;
		int len = sizeof(subAddress) + sizeof(LEP_UINT16) * dataLength;
		LEP_UINT8 *data = (LEP_UINT8 *)malloc(len);
		LEP_UINT8 *pos;

		if (data == NULL)
			return LEP_ERROR;

		pos = data;
		*pos++ = (LEP_UINT8)(subAddress >> 8);
		*pos++ = (LEP_UINT8)subAddress;

		for (int i = 0; i < dataLength; i++) {
			LEP_UINT16 temp = dataPtr[i];
			*pos++ = (LEP_UINT8)(temp >> 8);
			*pos++ = (LEP_UINT8)temp;
		}

		error = wire->write(deviceAddress << 1, (char *)data, len);
		if (error != 0)
			result = LEP_ERROR_I2C_FAIL;

		free(data);

		return result;
	}

	LEP_RESULT LEP_I2C_MasterReadRegister(LEP_PORTID portID,
		LEP_UINT8  deviceAddress,
		LEP_UINT16 regAddress,
		LEP_UINT16 *regValue)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;
		int error;
		LEP_UINT8 data[2], *pos;

		pos = data;
		*pos++ = (LEP_UINT8)(regAddress >> 8);
		*pos++ = (LEP_UINT8)regAddress;

		error = wire->write(deviceAddress << 1, (char *)data, sizeof(regAddress));
		if (error != 0)
			return LEP_ERROR_I2C_FAIL;

		error = wire->read(deviceAddress << 1, (char *)data, sizeof(*regValue));
		if (error != 0)
			return LEP_ERROR_I2C_FAIL;

		*regValue = (data[0] << 8) | data[1];

		return result;
	}

	LEP_RESULT LEP_I2C_MasterWriteRegister(LEP_PORTID portID,
		LEP_UINT8  deviceAddress,
		LEP_UINT16 regAddress,
		LEP_UINT16 regValue)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;
		int error;
		LEP_UINT8 data[sizeof(regAddress) + sizeof(regValue)];
		LEP_UINT8 *pos;

		if (data == NULL)
			return LEP_ERROR;

		pos = data;
		*pos++ = (LEP_UINT8)(regAddress >> 8);
		*pos++ = (LEP_UINT8)regAddress;

		*pos++ = (LEP_UINT8)(regValue >> 8);
		*pos++ = (LEP_UINT8)regValue;

		error = wire->write(deviceAddress << 1, (char *)data, sizeof(data));
		if (error != 0)
			result = LEP_ERROR_I2C_FAIL;

		return result;
	}

	LEP_RESULT LEP_I2C_MasterStatus(LEP_PORTID portID,
		LEP_UINT16 *portStatus)
	{
		mbed::I2C *wire = (mbed::I2C *)portID;
		LEP_RESULT result = LEP_OK;

		(void)wire;
		*portStatus = 0;

		return result;
	}

}
