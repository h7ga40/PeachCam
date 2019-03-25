
#ifndef _MEDIATASK_H_
#define _MEDIATASK_H_

#include <list>
#include "TaskBase.h"
#include "EasyAttach_CameraAndLCD.h"
#include "opencv.hpp"
#include "camera_if.hpp"
#include "face_detector.hpp"
#include "AUDIO_GRBoard.h"

struct mail_t {
	void *p_data;
	int result;
};

class MediaTask;

class AudioTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Idle,
			Recording,
		};
	};
public:
	AudioTask(MediaTask *owner, cv::Rect *face_roi);
	virtual ~AudioTask();
private:
	MediaTask *_owner;
	AUDIO_GRBoard audio;
	State::T _state;
	FILE *wav_fp;
	int pcm_size;
	bool _rec_signal;
	rbsp_data_conf_t audio_read_data;
	rbsp_data_conf_t audio_write_data;
	int _out_ridx;
	int _out_widx;
	FILE *shutter_fp;
	std::list<mail_t> mails;
	cv::Rect *_face_roi;
private:
	void AudioReadEnd(void *p_data, int result);
	static void callback_audio_read_end(void *p_data, int32_t result, void *p_app_data) {
		((AudioTask *)p_app_data)->AudioReadEnd(p_data, result);
	}
	void AudioWriteEnd(void *p_data, int result);
	static void callback_audio_write_end(void *p_data, int32_t result, void *p_app_data) {
		((AudioTask *)p_app_data)->AudioWriteEnd(p_data, result);
	}
	static void wire_data_4byte(uint32_t data, FILE *fp);
public:
	State::T GetState() { return _state; }
	void OnStart() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

class VisualTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Viewing,
			Recording,
		};
	};
public:
	VisualTask(MediaTask *owner);
	virtual ~VisualTask();
private:
	MediaTask *_owner;
	State::T _state;
public:
	State::T GetState() { return _state; }
	void OnStart() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

class VibratorTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			Off,
			On,
		};
	};
public:
	VibratorTask(MediaTask *owner);
	virtual ~VibratorTask();
private:
	MediaTask *_owner;
	State::T _state;
	mbed::DigitalOut _vibrator;
public:
	State::T GetState() { return _state; }
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

class GlobalState;

class MediaTask : public TaskThread
{
public:
	MediaTask(GlobalState *globalState, cv::Rect *face_roi);
	virtual ~MediaTask();
private:
	Tasks _task;
	ITask *_tasks[3];
	GlobalState *_globalState;
	AudioTask audioTask;
	VisualTask visualTask;
	VibratorTask vibratorTask;
public:
	std::string GetFilePath();
	bool IsActive();
	bool IsRecording() {
		return (audioTask.GetState() == AudioTask::State::Recording)
			|| (visualTask.GetState() == VisualTask::State::Recording);
	}
	void RecAudio();
	void StartShutter();
	void EndShutter();
	void UpdateRequest();
};

class FaceDetectTask : public TaskThread, public ITask
{
public:
	class State
	{
	public:
		enum T {
			PowerOff,
			Viewing,
			Detecting,
		};
	};
public:
	FaceDetectTask(GlobalState *globalState);
	virtual ~FaceDetectTask();
private:
	GlobalState *_globalState;
	State::T _state;
	int _timer;
	cv::Mat frame_gray;
	std::string _filename;
public:
	cv::Rect face_roi;
	void Init(std::string filename);
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
