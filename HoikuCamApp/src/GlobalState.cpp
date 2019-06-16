#include "mbed.h"
#include "GlobalState.h"
#include "SensorTask.h"
#include "NetTask.h"
#include "MediaTask.h"
#include "ZXingTask.h"

GlobalState::GlobalState() :
	storage(NULL),
	wifi(NULL),
	sensorTask(NULL),
	netTask(NULL),
	mediaTask(NULL),
	//faceDetectTask(NULL),
	leptonTask(NULL),
	zxingTask(NULL)
{
}

GlobalState::~GlobalState()
{
}

bool GlobalState::IsActive()
{
	return /*netTask->IsActive() ||*/ sensorTask->IsActive() || mediaTask->IsActive();
}

void GlobalState::MakeFilePath()
{
	char path[] = "1:/DCIM/YYYYMMDD";
	char file[] = "YYYYMMDDhhmmss";
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);

	mkdir("1:/DCIM", (mode_t)0000777);

	sprintf(path, "1:/DCIM/%04d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	mkdir(path, (mode_t)0000777);

	sprintf(file, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	_path = std::string(path) + "/" + std::string(file);
}

std::string GlobalState::GetFilePath()
{
	return _path;
}

void GlobalState::UpdateRequest()
{
	printf("GlobalState::UpdateRequest\r\n");
	netTask->Signal(InterTaskSignals::UpdateRequest);
}

void GlobalState::UploadRequest(std::string filename)
{
	printf("GlobalState::UploadRequest\r\n");
	netTask->UploadRequest(filename);
}

void GlobalState::PowerOff()
{
	printf("GlobalState::PowerOff\r\n");
	mediaTask->Signal(InterTaskSignals::PowerOff);
	sensorTask->Signal(InterTaskSignals::PowerOff);
	//faceDetectTask->Signal(InterTaskSignals::PowerOff);
	leptonTask->Signal(InterTaskSignals::PowerOff);
	zxingTask->Signal(InterTaskSignals::PowerOff);
}

void GlobalState::PowerOn()
{
	printf("GlobalState::PowerOn\r\n");
	mediaTask->Signal(InterTaskSignals::PowerOn);
	sensorTask->Signal(InterTaskSignals::PowerOn);
	//faceDetectTask->Signal(InterTaskSignals::PowerOn);
	leptonTask->Signal(InterTaskSignals::PowerOn);
	zxingTask->Signal(InterTaskSignals::PowerOn);
}

void GlobalState::TriggerOn()
{
	printf("GlobalState::TriggerOn\r\n");
	mediaTask->Signal(InterTaskSignals::TriggerOn);
}
