// HoikuCam

#include "mbed.h"
#include "SdUsbConnect.h"
#include "GlobalState.h"
#include "MediaTask.h"
#include "NetTask.h"
#include "SensorTask.h"
#include <expat.h>
#include "draw_font.h"
#include "bh1792.h"
#include "TouchKey.h"
#include "EasyAttach_CameraAndLCD.h"

#define FACE_DETECTOR_MODEL     "1:/lbpcascade_frontalface.xml"
#define TOUCH_NUM               (1u)

/* Application variables */
Mat frame_gray;     // Input frame (in grayscale)

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

static GlobalState globalState;
static SdUsbConnect sdUsbConnect("storage");
static FaceDetectTask faceDetectTask(&globalState);
static MediaTask mediaTask(&globalState, &faceDetectTask.face_roi);
static SensorTask sensorTask(&globalState);
ESP32Interface wifi(P3_10, P3_9, P2_14, P2_15);
static NetTask netTask(&globalState, &wifi);
static LeptonTaskThread leptonTask(&globalState);

enum parse_state_t {
	psRoot,
	psWifi,
	psWifiSsid,
	psWifiPassword,
	psWifiHostName,
	psUpload,
	psUploadServer,
	psUploadStorage,
	psError
};

struct wifi_config_t {
	char ssid[32];
	char password[32];
	char host_name[32];
};

struct upload_config_t {
	char server[32];
	char storage[32];
};

struct config_data_t {
	parse_state_t state;
	int pos;
	wifi_config_t wifi;
	upload_config_t upload;
};

static void XMLCALL
start(void *data, const XML_Char *el, const XML_Char **attr)
{
	config_data_t *cd = (config_data_t *)data;

	switch (cd->state) {
	case psRoot:
		if (strcmp(el, "wifi") == 0) {
			cd->state = psWifi;
		}
		else if (strcmp(el, "upload") == 0) {
			cd->state = psUpload;
		}
		break;
	case psWifi:
		if (strcmp(el, "ssid") == 0) {
			cd->wifi.ssid[0] = '\0';
			cd->pos = 0;
			cd->state = psWifiSsid;
		}
		else if (strcmp(el, "password") == 0) {
			cd->wifi.password[0] = '\0';
			cd->pos = 0;
			cd->state = psWifiPassword;
		}
		else if (strcmp(el, "host_name") == 0) {
			cd->wifi.host_name[0] = '\0';
			cd->pos = 0;
			cd->state = psWifiHostName;
		}
		break;
	case psUpload:
		if (strcmp(el, "server") == 0) {
			cd->upload.server[0] = '\0';
			cd->pos = 0;
			cd->state = psUploadServer;
		}
		else if (strcmp(el, "storage") == 0) {
			cd->upload.storage[0] = '\0';
			cd->pos = 0;
			cd->state = psUploadStorage;
		}
		break;
	default:
		break;
	}
}

static void XMLCALL
end(void *data, const XML_Char *el)
{
	config_data_t *cd = (config_data_t *)data;
	(void)el;

	switch (cd->state) {
	case psRoot:
		break;
	case psWifi:
		if (strcmp(el, "wifi") == 0) {
			cd->state = psRoot;
		}
		break;
	case psWifiSsid:
		if (strcmp(el, "ssid") == 0) {
			cd->state = psWifi;
		}
		break;
	case psWifiPassword:
		if (strcmp(el, "password") == 0) {
			cd->state = psWifi;
		}
		break;
	case psWifiHostName:
		if (strcmp(el, "host_name") == 0) {
			cd->state = psWifi;
		}
		break;
	case psUpload:
		if (strcmp(el, "upload") == 0) {
			cd->state = psRoot;
		}
		break;
	case psUploadServer:
		if (strcmp(el, "server") == 0) {
			cd->state = psUpload;
		}
		break;
	case psUploadStorage:
		if (strcmp(el, "storage") == 0) {
			cd->state = psUpload;
		}
		break;
	default:
		break;
	}
}

static void XMLCALL
text(void *data, const XML_Char *s, int len)
{
	config_data_t *cd = (config_data_t *)data;
	int l, maxlen;

	switch (cd->state) {
	case psRoot:
	case psWifi:
		break;
	case psWifiSsid:
		maxlen = sizeof(cd->wifi.ssid) - 1;
		l = cd->pos + len;
		if (l > maxlen)
			len = maxlen - cd->pos;

		if (len > 0) {
			memcpy(cd->wifi.ssid + cd->pos, s, len);
			cd->pos += len;
			cd->wifi.ssid[cd->pos] = '\0';
		}
		break;
	case psWifiPassword:
		maxlen = sizeof(cd->wifi.password) - 1;
		l = cd->pos + len;
		if (l > maxlen)
			len = maxlen - cd->pos;

		if (len > 0) {
			memcpy(cd->wifi.password + cd->pos, s, len);
			cd->pos += len;
			cd->wifi.password[cd->pos] = '\0';
		}
		break;
	case psWifiHostName:
		maxlen = sizeof(cd->wifi.host_name) - 1;
		l = cd->pos + len;
		if (l > maxlen)
			len = maxlen - cd->pos;

		if (len > 0) {
			memcpy(cd->wifi.host_name + cd->pos, s, len);
			cd->pos += len;
			cd->wifi.host_name[cd->pos] = '\0';
		}
		break;
	case psUploadServer:
		maxlen = sizeof(cd->upload.server) - 1;
		l = cd->pos + len;
		if (l > maxlen)
			len = maxlen - cd->pos;

		if (len > 0) {
			memcpy(cd->upload.server + cd->pos, s, len);
			cd->pos += len;
			cd->upload.server[cd->pos] = '\0';
		}
		break;
	case psUploadStorage:
		maxlen = sizeof(cd->upload.storage) - 1;
		l = cd->pos + len;
		if (l > maxlen)
			len = maxlen - cd->pos;

		if (len > 0) {
			memcpy(cd->upload.storage + cd->pos, s, len);
			cd->pos += len;
			cd->upload.storage[cd->pos] = '\0';
		}
		break;
	default:
		break;
	}
}

config_data_t config;
int8_t temp[256];

bool ReadIniFile(std::string filename)
{
	XML_Parser parser;

	memset(&config, 0, sizeof(config));
	parser = XML_ParserCreate(NULL);
	if (!parser) {
		printf("Couldn't allocate memory for parser\n");
		return false;
	}

	XML_SetUserData(parser, &config);
	XML_SetElementHandler(parser, start, end);
	XML_SetCharacterDataHandler(parser, text);

	size_t len;
	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp != NULL) {
		for (;;) {
			len = fread(temp, 1, sizeof(temp), fp);
			if (len <= 0)
				break;

			if ((config.state != psError) && (XML_Parse(parser, (char *)temp, len, 0) == 0)) {
				XML_Error error_code = XML_GetErrorCode(parser);
				printf("Parsing response buffer of size %lid failed"
					" with error code %d (%s).\n",
					len, error_code, XML_ErrorString(error_code));
				config.state = psError;
			}
		}

		fclose(fp);
	}

	XML_ParserFree(parser);

	return true;
}

int main()
{
	char textbuf[80];
	globalState.storage = &sdUsbConnect;
	globalState.wifi = &wifi;
	globalState.netTask = &netTask;
	globalState.mediaTask = &mediaTask;
	globalState.faceDetectTask = &faceDetectTask;
	globalState.sensorTask = &sensorTask;
	globalState.leptonTask = &leptonTask;

	uint8_t touch_num = 0;
	TouchKey::touch_pos_t touch_pos[TOUCH_NUM];

	/* Reset touch IC */
	TouckKey_LCD_shield touch(P4_0, P2_13, I2C_SDA, I2C_SCL);
	touch.Reset();

	globalState.storage->wait_connect();

	ReadIniFile("1:/setting.xml");

	netTask.Init(config.wifi.ssid, config.wifi.password, config.wifi.host_name,
		config.upload.server, config.upload.storage);
	faceDetectTask.Init(FACE_DETECTOR_MODEL);

	netTask.Start();
	sensorTask.Start();
	mediaTask.Start();
	faceDetectTask.Start();
	leptonTask.Start();

	us_timestamp_t now, org = ticker_read_us(get_us_ticker_data());
	while (true) {
		now = ticker_read_us(get_us_ticker_data());
		int diff = now - org;
		if (diff > 1000000) {
			org += (diff / 1000000) * 1000000;
			led1 = !led1;

			std::string temp;
			time_t tm = time(NULL);

			if (netTask.IsConnected())
				temp = std::string(config.wifi.ssid) + std::string(" Connected    ");
			else
				temp = std::string("Connecting to ") + std::string(config.wifi.ssid);
			lcd_drawString(temp.c_str(), 0, 248, 0xFCCC, 0x0000);

			if (netTask.IsDetectedServer())
				temp = std::string(config.upload.server) + std::string(" detected, IP address ") + std::string(netTask.GetServerAddr().get_ip_address());
			else
				temp = std::string(config.upload.server) + std::string(" no detected                          ");
			lcd_drawString(temp.c_str(), 0, 260, 0xFCCC, 0x0000);

			temp = std::string(ctime(&tm));
			lcd_drawString(temp.c_str(), 330, 0, 0xFCCC, 0x0000);
#if 0
			uint16_t *data = leptonTask.GetTelemetryA();
			snprintf(textbuf, sizeof(textbuf), "TmA:%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
			lcd_drawString(textbuf, 72, 148, 0xFCCC, 0x0000);

			data = leptonTask.GetTelemetryB();
			snprintf(textbuf, sizeof(textbuf), "TmB:%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
			lcd_drawString(textbuf, 72, 160, 0xFCCC, 0x0000);

			data = leptonTask.GetTelemetryC();
			snprintf(textbuf, sizeof(textbuf), "TmC:%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
			lcd_drawString(textbuf, 72, 172, 0xFCCC, 0x0000);
#endif
			uint16_t fpatem = leptonTask.GetFpaTemperature();
			snprintf(textbuf, sizeof(textbuf), "FPA:%3.2f℃", fpatem / 100.0 - 273.15);
			lcd_drawString(textbuf, 400, 172, 0xFCCC, 0x0000);

			uint16_t minValue, maxValue;
			minValue = leptonTask.GetMinValue();
			maxValue = leptonTask.GetMaxValue();
			snprintf(textbuf, sizeof(textbuf), "min:%3.2f℃", 0.026 * (minValue - 8192) + fpatem / 100.0 - 273.15);
			lcd_drawString(textbuf, 400, 184, 0xFCCC, 0x0000);
			snprintf(textbuf, sizeof(textbuf), "max:%3.2f℃", 0.026 * (maxValue - 8192) + fpatem / 100.0 - 273.15);
			lcd_drawString(textbuf, 400, 196, 0xFCCC, 0x0000);
		}
		/* Get coordinates */
		touch_num = touch.GetCoordinates(TOUCH_NUM, touch_pos);
		if (touch_num != 0)
			snprintf(textbuf, sizeof(textbuf), "x:%4d, y:%4d", touch_pos[0].x, touch_pos[0].y);
		else
			strncpy(textbuf, "              ", sizeof(textbuf));
		lcd_drawString(textbuf, 0, 100, 0xFCCC, 0x0000);

		led2 = netTask.IsConnected();
		led3 = netTask.IsActive();
		led4 = mediaTask.IsRecording();

		ThisThread::sleep_for((diff / 1000) % 100);
	}

	return 0;
}
