#include "mbed.h"
#include "MediaTask.h"
#include "GlobalState.h"
#include "DisplayBase.h"
#include "draw_font.h"
#include "platform/CriticalSectionLock.h"
#include "JPEG_Converter.h"

using namespace cv;

#define AUDIO_FRAME_BUFFER_STRIDE    (((LCD_PIXEL_WIDTH * 2) + 31u) & ~31u)
#define AUDIO_FRAME_BUFFER_HEIGHT    (LCD_PIXEL_HEIGHT)

static uint8_t audio_frame_buffer[AUDIO_FRAME_BUFFER_STRIDE * AUDIO_FRAME_BUFFER_HEIGHT]__attribute((section("NC_BSS"),aligned(32)));

DisplayBase Display;
JPEG_Converter Jcu;

void Audio_Start_LCD_Display(void) {
    DisplayBase::rect_t rect;

    memset(audio_frame_buffer, 0, sizeof(audio_frame_buffer));

    rect.vs = 0;
    rect.vw = LCD_PIXEL_HEIGHT;
    rect.hs = 0;
    rect.hw = LCD_PIXEL_WIDTH;
    Display.Graphics_Read_Setting(
        DisplayBase::GRAPHICS_LAYER_3,
        (void *)audio_frame_buffer,
        AUDIO_FRAME_BUFFER_STRIDE,
        DisplayBase::GRAPHICS_FORMAT_ARGB4444,
        DisplayBase::WR_RD_WRSWA_32_16BIT,
        &rect
    );
    Display.Graphics_Start(DisplayBase::GRAPHICS_LAYER_3);
}

void clear_screen(void) {
	memset(audio_frame_buffer, 0, sizeof(audio_frame_buffer));
}

void draw_square(int x, int y, int w, int h, uint32_t colour) {
	uint16_t * p_bottom_left_pos = (uint16_t *)&audio_frame_buffer[0];
	int idx_base;
	int wk_idx;
	int i;

	idx_base = (x + (LCD_PIXEL_WIDTH * y));

	/* top */
	wk_idx = idx_base;
	for (i = 0; i < w; i++) {
		p_bottom_left_pos[wk_idx++] = colour;
	}

	/* middle */
	for (i = 1; i < (h - 1); i++) {
		wk_idx = idx_base + (LCD_PIXEL_WIDTH * i);
		p_bottom_left_pos[wk_idx] = colour;
		wk_idx += (w - 1);
		p_bottom_left_pos[wk_idx] = colour;
	}

	/* bottom */
	wk_idx = idx_base + (LCD_PIXEL_WIDTH * (h - 1));
	for (i = 0; i < w; i++) {
		p_bottom_left_pos[wk_idx++] = colour;
	}
}

static void disp_audio_wave(int16_t * p_data, int32_t size, uint32_t color) {
    uint16_t * p_bottom_left_pos = (uint16_t *)&audio_frame_buffer[AUDIO_FRAME_BUFFER_STRIDE * (AUDIO_FRAME_BUFFER_HEIGHT - 1)];
    uint16_t * p_frame_buf;
    uint32_t x = 0;
    uint32_t data_pos;
    uint32_t data_pos_last = ((p_data[0] + 0x8000ul) >> 8) + 8;
    int loop_num;

    if (size < 0) {
        return;
    }

    for (int i = 0; i < size; i += 2) {
        data_pos = ((p_data[i] + 0x8000ul) >> 8) + 8;
        p_frame_buf = &p_bottom_left_pos[x];

        if (data_pos == data_pos_last) {
            loop_num = 3;
            p_frame_buf -= ((data_pos - 1) * LCD_PIXEL_WIDTH);
        }
        else if (data_pos > data_pos_last) {
            loop_num = data_pos - data_pos_last + 2;
            p_frame_buf -= (data_pos_last * LCD_PIXEL_WIDTH);
        }
        else {
            loop_num = data_pos_last - data_pos + 2;
            p_frame_buf -= ((data_pos - 1) * LCD_PIXEL_WIDTH);
        }

        for (int j = 0; j < loop_num; j++) {
            *p_frame_buf = color;
            p_frame_buf -= LCD_PIXEL_WIDTH;
        }
        data_pos_last = data_pos;

        x++;
        if (x >= LCD_PIXEL_WIDTH) {
            break;
        }
    }
}

#define WAVE_FORMAT_PCM			(0x0001)
#define SAMPLE_RATE				(16000)
#define CHANNEL_NUM				(1)
#define DATA_SPEED				(SAMPLE_RATE * CHANNEL_NUM * 2)
#define AUDIO_IN_BUF_SIZE		(2048)
#define AUDIO_IN_BUF_NUM		(16)
#define AUDIO_OUT_BUF_SIZE		(2048)
#define AUDIO_OUT_BUF_NUM		(8)

static uint8_t audio_in_buf[AUDIO_IN_BUF_NUM][AUDIO_IN_BUF_SIZE]__attribute((section("NC_BSS"), aligned(4)));
static uint8_t audio_out_buf[AUDIO_OUT_BUF_NUM][AUDIO_OUT_BUF_SIZE]__attribute((section("NC_BSS"), aligned(4)));
#if CHANNEL_NUM == 1
static uint8_t audio_file_buf[AUDIO_IN_BUF_SIZE / 2];
#endif
uint8_t heart_mark;

#define LE_2BYTE_NUM(x)			(x & 0xFF),((x >> 8) & 0xFF)
#define LE_4BYTE_NUM(x)			(x & 0xFF),((x >> 8) & 0xFF),((x >> 16) & 0xFF),((x >> 24) & 0xFF)

// wav file header
static const unsigned char wav_header_tbl[] = {
	'R','I','F','F',
	// ファイルサイズ
	0x00,0x00,0x00,0x00,
	'W','A','V','E',
	'f','m','t',' ',
	// fmt チャンクのバイト数
	0x10,0x00,0x00,0x00,
	// リニアPCM
	LE_2BYTE_NUM(WAVE_FORMAT_PCM),
	// チャネル数
	LE_2BYTE_NUM(CHANNEL_NUM),
	// サンプリングレート
	LE_4BYTE_NUM(SAMPLE_RATE),
	// データ速度 (Byte/sec)
	LE_4BYTE_NUM(DATA_SPEED),
	// ブロックサイズ (Byte/sample×チャンネル数)
	LE_2BYTE_NUM(2),
	// サンプルあたりのビット数 (bit/sample)
	LE_2BYTE_NUM(16),
	'd','a','t','a',
	// 波形データのバイト数
	0x00,0x00,0x00,0x00
};

AudioTask::AudioTask(MediaTask *owner, cv::Rect *face_roi) :
	Task(osWaitForever),
	_owner(owner),
	audio(0x80, AUDIO_OUT_BUF_NUM - 1, AUDIO_IN_BUF_NUM),
	_state(State::PowerOff),
	wav_fp(NULL),
	pcm_size(0),
	_rec_signal(false),
	audio_read_data(),
	audio_write_data(),
	_out_ridx(0),
	_out_widx(0),
	shutter_fp(NULL),
	mails(),
	_face_roi(face_roi)
{
}

AudioTask::~AudioTask()
{

}

void AudioTask::OnStart()
{
	audio_read_data.p_app_data = this;
	audio_read_data.p_notify_func = callback_audio_read_end;

	for (int i = 0; i < AUDIO_IN_BUF_NUM; i++) {
		if (audio.read(audio_in_buf[i], AUDIO_IN_BUF_SIZE, &audio_read_data) < 0) {
			printf("read error\n");
		}
	}

	audio_write_data.p_app_data = this;
	audio_write_data.p_notify_func = callback_audio_write_end;
}

void AudioTask::AudioReadEnd(void *p_data, int result)
{
	uint32_t color;

	if (_state == State::Recording) {
		color = 0xFF00; // Red
	}
	else {
		color = 0xF00F; // Blue
	}
	clear_screen();
	disp_audio_wave((int16_t *)p_data, result / 2, color);
	if (_face_roi->width > 0 && _face_roi->height > 0) {
		draw_square(_face_roi->x, _face_roi->y, _face_roi->width, _face_roi->height, 0xF0F0);
	}
	lcd_drawString("●", 0, 0, 0xF000 | (0xF00 & (((uint16_t)heart_mark) << 8)), 0x0000);

	mail_t mail = {
		.p_data = p_data,
		.result = result,
	};

	{
		CriticalSectionLock mutex();
		mails.push_back(mail);
	}

	_owner->RecAudio();
}

void AudioTask::AudioWriteEnd(void *p_data, int result)
{
	(void)p_data;
	(void)result;

	if (_out_ridx == _out_widx) {
		_owner->EndShutter();
		return;
	}

	_out_ridx++;
	if (_out_ridx >= AUDIO_OUT_BUF_NUM) {
		_out_ridx = 0;
	}

	_owner->StartShutter();
}

void AudioTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::PowerOn) != 0) {
		audio.micVolume(0.60f);
		audio.power(true);
		audio.frequency(SAMPLE_RATE);

		_state = State::Idle;
		_timer = osWaitForever;
	}
	if ((signals & InterTaskSignals::TriggerOn) != 0) {
		auto file = _owner->GetFilePath() + ".wav";
		wav_fp = fopen(file.c_str(), "wb");
		if (wav_fp != NULL) {
			pcm_size = 0;
			fwrite(wav_header_tbl, sizeof(char), sizeof(wav_header_tbl), wav_fp);
			_state = State::Recording;
			_timer = 10 * 1000;
		}
		file = "1:/shutter.wav";
		shutter_fp = fopen(file.c_str(), "rb");
		if (shutter_fp != NULL) {
			audio.outputVolume(1.00, 1.00);
			fseek(shutter_fp, sizeof(wav_header_tbl), SEEK_SET);
			signals = (InterTaskSignals::T)(signals | InterTaskSignals::StartShutter);
			printf("StartShutter w%d == r%d\n", _out_widx, _out_ridx);
		}
	}
	if ((signals & InterTaskSignals::RecAudio) != 0) {
		_rec_signal = true;
	}
	if ((signals & InterTaskSignals::StartShutter) != 0) {
		int ridx = _out_ridx;
		if (ridx <= 0) ridx = AUDIO_OUT_BUF_NUM - 1; else ridx--;
		if (shutter_fp != NULL) do {
			uint8_t *buf = audio_out_buf[_out_widx];
			size_t size = fread(buf, sizeof(char), AUDIO_OUT_BUF_SIZE, shutter_fp);
			if (size == 0) {
				fclose(shutter_fp);
				shutter_fp = NULL;
				break;
			}
			else {
				if (size < AUDIO_OUT_BUF_SIZE) {
					memset(&buf[size], 0, AUDIO_OUT_BUF_SIZE - size);
				}
				audio.write(buf, AUDIO_OUT_BUF_SIZE, &audio_write_data);
				_out_widx++;
				if (_out_widx >= AUDIO_OUT_BUF_NUM) {
					_out_widx = 0;
				}
			}
		} while (_out_widx != ridx);
	}
	if ((signals & InterTaskSignals::EndShutter) != 0) {
		printf("EndShutter w%d == r%d\n", _out_widx, _out_ridx);
		audio.outputVolume(0.0, 0.0);
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		if (_state == State::Idle) {
			audio.micVolume(0.0);
			audio.power(false);

			_state = State::PowerOff;
			_timer = osWaitForever;
		}
	}
}

void AudioTask::Process()
{
	bool ret = false;
	mail_t mail;

	if (_rec_signal) {
		{
			CriticalSectionLock mutex();
			ret = !mails.empty();
			if (ret) {
				mail = mails.front();
				mails.pop_front();
			}
		}

		if (ret) {
			audio.read(mail.p_data, AUDIO_IN_BUF_SIZE, &audio_read_data);
		}

		if (!mails.empty())
			_owner->RecAudio();
	}

	switch (_state) {
	case State::PowerOff:
	case State::Idle:
		break;
	case State::Recording:
		if (ret) {
			if (wav_fp != NULL) {
#if CHANNEL_NUM == 1
				int16_t *src = (int16_t *)mail.p_data, *end = &src[mail.result / sizeof(int16_t)];
				int16_t *dst = (int16_t *)audio_file_buf;
				// 2chデータを1chに変換
				for (; src < end; src += 2, dst++) {
					*dst = *src;;
				}
				int len = mail.result / 2;
				pcm_size += len;
				fwrite(audio_file_buf, sizeof(char), len, wav_fp);
#else
				pcm_size += mail.result;
				fwrite(mail.p_data, sizeof(char), mail.result, wav_fp);
#endif
			}
		}
		if (_timer == 0) {
			if (wav_fp != NULL) {
				// Set "RIFF" ChunkSize
				fseek(wav_fp, 4, SEEK_SET);
				wire_data_4byte(sizeof(wav_header_tbl) - 8 + pcm_size, wav_fp);
				// Set "data" ChunkSize
				fseek(wav_fp, 40, SEEK_SET);
				wire_data_4byte(pcm_size, wav_fp);
				fclose(wav_fp);
				wav_fp = NULL;
			}

			_owner->UpdateRequest();

			_state = State::Idle;
			_timer = osWaitForever;
		}
		break;
	}

	if (_timer == 0)
		_timer = osWaitForever;
}

void AudioTask::wire_data_4byte(uint32_t data, FILE *fp)
{
	uint8_t work_buf[4];

	work_buf[0] = (uint8_t)(data >> 0);
	work_buf[1] = (uint8_t)(data >> 8);
	work_buf[2] = (uint8_t)(data >> 16);
	work_buf[3] = (uint8_t)(data >> 24);

	fwrite(work_buf, sizeof(char), sizeof(work_buf), fp);
}

VisualTask::VisualTask(MediaTask *owner) :
	Task(osWaitForever),
	_owner(owner),
	_state(State::PowerOff)
{
}

VisualTask::~VisualTask()
{
}

void VisualTask::OnStart()
{
	camera_start();
	Audio_Start_LCD_Display();

	Display.Video_Stop(DisplayBase::VIDEO_INPUT_CHANNEL_0);
	Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_0);
	Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_2);
	Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_3);
}

void VisualTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::PowerOn) != 0) {
		Display.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);
		Display.Graphics_Start(DisplayBase::GRAPHICS_LAYER_0);
		Display.Graphics_Start(DisplayBase::GRAPHICS_LAYER_2);
		Display.Graphics_Start(DisplayBase::GRAPHICS_LAYER_3);
		EasyAttach_LcdBacklight(true);

		_state = State::Viewing;
		_timer = osWaitForever;
	}
	if ((signals & InterTaskSignals::TriggerOn) != 0) {
		State::T temp = _state;
		if (temp != State::PowerOff) {
			_state = State::Recording;

			size_t jpeg_size = create_jpeg();
			auto file = _owner->GetFilePath() + ".jpeg";
			FILE *jpeg_fp = fopen(file.c_str(), "wb");
			if (jpeg_fp != NULL) {
				fwrite(get_jpeg_adr(), sizeof(char), jpeg_size, jpeg_fp);
				fclose(jpeg_fp);
				jpeg_fp = NULL;
			}

			_state = temp;
			_timer = osWaitForever;
		}
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		if (_state == State::Viewing) {
			EasyAttach_LcdBacklight(false);
			Display.Video_Stop(DisplayBase::VIDEO_INPUT_CHANNEL_0);
			Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_0);
			Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_2);
			Display.Graphics_Stop(DisplayBase::GRAPHICS_LAYER_3);

			_state = State::PowerOff;
			_timer = osWaitForever;
		}
	}
}

void VisualTask::Process()
{
	_timer = osWaitForever;
}

VibratorTask::VibratorTask(MediaTask *owner) :
	Task(osWaitForever),
	_state(State::Off),
	_vibrator(P5_10)
{
}

VibratorTask::~VibratorTask()
{
}

void VibratorTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::TriggerOn) != 0) {
		_vibrator = 1;
		_state = State::On;
		_timer = 10;
	}
}

void VibratorTask::Process()
{
	if (_timer != 0)
		return;

	_vibrator = 0;
	_state = State::Off;
	_timer = osWaitForever;
}

MediaTask::MediaTask(GlobalState *globalState, cv::Rect *face_roi) :
	TaskThread(&_task),
	_task(_tasks, sizeof(_tasks) / sizeof(_tasks[0])),
	_globalState(globalState),
	audioTask(this, face_roi),
	visualTask(this),
	vibratorTask(this)
{
	_tasks[0] = &visualTask;
	_tasks[1] = &audioTask;
	_tasks[2] = &vibratorTask;
}

MediaTask::~MediaTask()
{
}

std::string MediaTask::GetFilePath()
{
	return _globalState->GetFilePath();
}

bool MediaTask::IsActive()
{
	return (audioTask.GetState() == AudioTask::State::Recording)
		|| (visualTask.GetState() == VisualTask::State::Recording);
}

void MediaTask::RecAudio()
{
	Signal(InterTaskSignals::RecAudio);
}

void MediaTask::StartShutter()
{
	Signal(InterTaskSignals::StartShutter);
}

void MediaTask::EndShutter()
{
	Signal(InterTaskSignals::EndShutter);
}

void MediaTask::UpdateRequest()
{
	_globalState->UpdateRequest();
}

FaceDetectTask::FaceDetectTask(GlobalState *globalState) :
	TaskThread(this, osPriorityBelowNormal, (1024 * 33), NULL, "FaceDetectTask"),
	_globalState(globalState),
	_state(State::PowerOff),
	_timer(0)
{
}

FaceDetectTask::~FaceDetectTask()
{
}

void FaceDetectTask::Init(std::string filename)
{
	_filename = filename;
}

void FaceDetectTask::OnStart()
{
	detectFaceInit(_filename);
}

void FaceDetectTask::OnEnd()
{
}

int FaceDetectTask::GetTimer()
{
	return _timer;
}

void FaceDetectTask::Progress(int elapse)
{
	_timer -= elapse;
	if (_timer < 0)
		_timer = 0;
}

void FaceDetectTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::PowerOn) != 0) {
		_state = State::Viewing;
		_timer = 100;
	}
	if ((signals & InterTaskSignals::PowerOff) != 0) {
		_state = State::PowerOff;
		_timer = osWaitForever;
	}
}

void FaceDetectTask::Process()
{
	if (_timer != 0)
		return;

	switch (_state) {
	case State::Viewing:
		create_gray(frame_gray);
		if (frame_gray.empty()){
			_state = State::Viewing;
			_timer = 100;
			break;
		}

		detectFace(frame_gray, face_roi);
		if (face_roi.width > 0 && face_roi.height > 0) {
			printf("FaceDetect: %d,%d,%d,%d\r\n", face_roi.x, face_roi.y, face_roi.width, face_roi.height);
			_state = State::Detecting;
			_timer = 1000;
		}
		else {
			_state = State::Viewing;
			_timer = 100;
		}
		break;
	case State::Detecting:
		face_roi.width = -1;
		face_roi.height = -1;

		_state = State::Viewing;
		_timer = 0;
		break;
	default:
		_state = State::PowerOff;
		_timer = osWaitForever;
		break;
	}
}
