
#include "mbed.h"
#include "EasyAttach_CameraAndLCD.h"
#include "ImageReaderSource.h"
#include "camera_if.hpp"
#include "ZXingTask.h"

extern uint8_t FrameBuffer_Video[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT];

/**** User Selection *********/
/** Decode hints **/
//#define DECODE_HINTS           (DecodeHints::ONED_HINT | DecodeHints::QR_CODE_HINT | DecodeHints::DATA_MATRIX_HINT | DecodeHints::AZTEC_HINT)
#define DECODE_HINTS           (DecodeHints::QR_CODE_HINT)
/*****************************/

extern DisplayBase Display;

ZXingTask::ZXingTask(GlobalState *globalState) :
	TaskThread(this, osPriorityBelowNormal, (1024 * 33), NULL, "ZXingTask"),
	_globalState(globalState),
	_state(State::PowerOff),
	_timer(0),
	p_callback_func(NULL)
{
}

ZXingTask::~ZXingTask()
{
}

void ZXingTask::Init(void (*pfunc)(const char *addr, int size))
{
	p_callback_func = pfunc;
}

void ZXingTask::OnStart()
{
}

void ZXingTask::OnEnd()
{
}

int ZXingTask::GetTimer()
{
	return _timer;
}

void ZXingTask::Progress(int elapse)
{
	_timer -= elapse;
	if (_timer < 0)
		_timer = 0;
}

void ZXingTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::PowerOn) != 0) {
		_state = State::Detecting;
		_timer = 100;
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		_state = State::PowerOff;
		_timer = osWaitForever;
	}
}

void ZXingTask::Process()
{
	if (_timer != 0)
		return;

	int decode_result;
	const char *decode_str;

	switch (_state) {
	case State::Detecting: {
		Ref<Result> results;
		DecodeHints hints(DECODE_HINTS);
		hints.setTryHarder(false);
		QRCodeReader reader;
		Ref<LuminanceSource> source;
		decode_result = ImageReaderSource::create((char *)FrameBuffer_Video, (FRAME_BUFFER_STRIDE * VIDEO_PIXEL_VW), VIDEO_PIXEL_HW, VIDEO_PIXEL_VW, source);
		if (decode_result == 0) {
			Ref<Binarizer> binarizer;
			binarizer = new GlobalHistogramBinarizer(source);
			Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
			decode_result = reader.decode(binary, hints, results);
		}
		if (decode_result == 0) {
			decode_str = results->getText()->getText().c_str();
			int size = strlen(decode_str);
			if (p_callback_func != NULL) {
				p_callback_func(decode_str, size);
			}
			_state = State::Detected;
			_timer = 500;
		}
		else {
			if (p_callback_func != NULL) {
				p_callback_func("", 0);
			}
			_state = State::Detecting;
			_timer = 10;
		}
		break;
	}
	case State::Detected:
		if (p_callback_func != NULL) {
			p_callback_func("", 0);
		}
		_state = State::Detecting;
		_timer = 0;
		break;
	default:
		_state = State::PowerOff;
		_timer = osWaitForever;
		break;
	}
}
