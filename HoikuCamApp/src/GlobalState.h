
#ifndef _GLOBALSTATE_H_
#define _GLOBALSTATE_H_

#include <string>

class SdUsbConnect;
class ESP32Interface;
class SensorTask;
class NetTask;
class MediaTask;
class FaceDetectTask;
class LeptonTaskThread;
class ZXingTask;

class GlobalState
{
public:
	GlobalState();
	virtual ~GlobalState();
public:
	SdUsbConnect *storage;
	ESP32Interface *wifi;
	SensorTask *sensorTask;
	NetTask *netTask;
	MediaTask *mediaTask;
	//FaceDetectTask *faceDetectTask;
	LeptonTaskThread *leptonTask;
	ZXingTask *zxingTask;
	std::string _path;
public:
	bool IsActive();
	void MakeFilePath();
	std::string GetFilePath();
	void UpdateRequest();
	void UploadRequest(std::string filename);
	void PowerOff();
	void PowerOn();
	void TriggerOn();
};

#endif // _GLOBALSTATE_H_
