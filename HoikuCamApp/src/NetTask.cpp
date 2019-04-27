#include "mbed.h"
#include "NetTask.h"
#include "http_request.h"

NtpTask::NtpTask(NetTask *owner) :
	Task(osWaitForever),
	_owner(owner),
	_state(State::Unsynced),
	_retry(0)
{
}

NtpTask::~NtpTask()
{
}

void NtpTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::WifiConnected) != 0) {
		_state = State::Unsynced;
		_timer = 0;
	}
}

void NtpTask::Process()
{
	if (_timer != 0)
		return;

	switch (_state) {
	case State::Unsynced:
		if (!_owner->IsConnected()) {
			_state = State::Unsynced;
			_timer = osWaitForever;
		}
		else if (_owner->InvokeNtp()) {
			_retry = 0;
			_state = State::Synced;
			_timer = 3 * 60 * 1000;
		}
		else {
			_retry++;
			if (_retry >= 3) {
				_retry = 0;
				_state = State::Unsynced;
				_timer = 1 * 60 * 1000;
			}
			else {
				_state = State::Unsynced;
				_timer = 10 * 1000;
			}
		}
		break;
	case State::Synced:
		_state = State::Unsynced;
		_timer = 0;
		break;
	default:
		_state = State::Unsynced;
		_timer = 10 * 1000;
		break;
	}
}

UploadTask::UploadTask(NetTask *owner) :
	Task(osWaitForever),
	_owner(owner),
	_state(),
	_retry(0),
	_server(),
	_serverAddr(),
	_storage(),
	_storageAddr(),
	_update_req(false),
	_uploads()
{
}

UploadTask::~UploadTask()
{
}

void UploadTask::Init(std::string server, std::string storage)
{
	_server = server;
	_serverAddr.set_ip_address(server.c_str());
	_storage = storage;
	_storageAddr.set_ip_address(storage.c_str());
}

void UploadTask::UploadRequest(std::string filename)
{
	_mutex.lock();
	_uploads.push_back(filename);
	_mutex.unlock();

	_owner->Signal(InterTaskSignals::UploadRequest);
}

void UploadTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::UpdateRequest) != 0) {
		_update_req = true;
		if (_serverAddr.get_ip_version() == NSAPI_UNSPEC) {
			_state = State::Undetected;
			_timer = 0;
		}
		else {
			_state = State::Update;
			_timer = 0;
		}
	}
	if ((signals & InterTaskSignals::UploadRequest) != 0) {
		if (_serverAddr.get_ip_version() == NSAPI_UNSPEC) {
			_state = State::Undetected;
			_timer = 0;
		}
		else {
			_state = State::Upload;
			_timer = 0;
		}
	}
}

bool UploadTask::Upload()
{
	bool result;
	std::string filename;

	_mutex.lock();
	result = !_uploads.empty();
	if (result) {
		filename = _uploads.front();
		_uploads.pop_front();
	}
	_mutex.unlock();

	if (result) {
		result = _owner->Upload(_serverAddr, filename);
	}
	if (!result) {
		_mutex.lock();
		_uploads.push_back(filename);
		_mutex.unlock();
	}

	return result;
}

void UploadTask::Process()
{
	SocketAddress addr;

	if (_timer != 0)
		return;

	switch (_state) {
	case State::Undetected:
		if (!_owner->IsConnected()) {
			_state = State::Undetected;
			_timer = osWaitForever;
		}
		else if (_owner->QuerySever(_server.c_str(), addr)) {
			_serverAddr.set_addr(addr.get_addr());
			_retry = 0;
			if (_update_req) {
				_state = State::Update;
				_timer = 0;
			}
			else {
				_state = State::Detected;
				_timer = 1 * 60 * 1000;
			}
		}
		else {
			_retry++;
			if (_retry >= 3) {
				_retry = 0;
				_state = State::Undetected;
				_timer = 1 * 60 * 1000;
			}
			else {
				_state = State::Undetected;
				_timer = 10 * 1000;
			}
		}
		break;
	case State::Detected:
		if (_serverAddr.get_ip_version() == NSAPI_UNSPEC) {
			_state = State::Undetected;
			_timer = 0;
		}
		else {
			_state = State::Detected;
			_timer = osWaitForever;
		}
		break;
	case State::Update:
		if (_owner->Update(_serverAddr, _storageAddr)) {
			_owner->WifiSleep(true);
			_update_req = false;
			_retry = 0;
			_state = State::Detected;
			_timer = osWaitForever;
		}
		else {
			_retry++;
			if (_retry >= 3) {
				if (_serverAddr.get_ip_version() == NSAPI_UNSPEC) {
					_serverAddr.set_addr(nsapi_addr_t());
					_retry = 0;
					_state = State::Undetected;
					_timer = 0;
				}
				else {
					_retry = 0;
					_state = State::Update;
					_timer = 3 * 60 * 1000;
				}
			}
			else {
				_state = State::Update;
				_timer = 10 * 1000;
			}
		}
		break;
	case State::Upload:
		if (Upload()) {
			_retry = 0;
			_state = State::Detected;
			_timer = osWaitForever;
		}
		else {
			_retry++;
			if (_retry >= 3) {
				if (_serverAddr.get_ip_version() == NSAPI_UNSPEC) {
					_serverAddr.set_addr(nsapi_addr_t());
					_retry = 0;
					_state = State::Undetected;
					_timer = 0;
				}
				else {
					_retry = 0;
					_state = State::Upload;
					_timer = 3 * 60 * 1000;
				}
			}
			else {
				_state = State::Upload;
				_timer = 10 * 1000;
			}
		}
		break;
	default:
		_state = State::Undetected;
		_timer = 0;
		break;
	}
}

WifiTask::WifiTask(NetTask *owner, ESP32Interface *wifi) :
	Task(osWaitForever),
	_owner(owner),
	_wifi(wifi),
	_ssid(),
	_password(),
	_state(State::Disconnected)
{
}

WifiTask::~WifiTask()
{

}

void WifiTask::Init(std::string ssid, std::string password, std::string host_name)
{
	_ssid = ssid;
	_password = password;
	_host_name = host_name;
}

void WifiTask::wifi_status(nsapi_event_t evt, intptr_t obj)
{
	_owner->WifiStatus(evt);
}

void WifiTask::OnStart()
{
	_wifi->attach(callback(this, &WifiTask::wifi_status));

	_state = State::Disconnected;
	if (!_ssid.empty() && !_password.empty()) {
		_timer = 1000;
	}
	else {
		_timer = 30 * 1000;
	}
}

void WifiTask::ProcessEvent(InterTaskSignals::T signals)
{
	if ((signals & InterTaskSignals::WifiStatusChanged) != 0) {
		State new_state;

		switch (_wifi->get_connection_status()) {
		case NSAPI_STATUS_LOCAL_UP:
		case NSAPI_STATUS_GLOBAL_UP:
			_owner->WifiConnected();

			_state = State::Connected;
			_timer = osWaitForever;
			break;
		case NSAPI_STATUS_DISCONNECTED:
			_state = State::Disconnected;
			_timer = 30 * 1000;
			break;
		case NSAPI_STATUS_CONNECTING:
			_state = State::Connecting;
			_timer = osWaitForever;
			break;
		case NSAPI_STATUS_ERROR_UNSUPPORTED:
		default:
			_state = State::Disconnected;
			_timer = 30 * 1000;
			break;
		}
	}
}

void WifiTask::Process()
{
	nsapi_connection_status_t ret;

	if (_timer != 0)
		return;

	switch (_state) {
	case State::Disconnected:
		if (!_ssid.empty() && !_password.empty()) {
			if (_wifi->connect(_ssid.c_str(), _password.c_str(), NSAPI_SECURITY_WPA_WPA2) == NSAPI_ERROR_OK) {
				_wifi->ntp(true, 0);
				_wifi->mdns(true, _host_name.c_str(), "_iot", 3601);

				_state = State::Connecting;
				_timer = 1000;
			}
			else {
				_state = State::Disconnected;
				_timer = 30 * 1000;
			}
		}
		else {
			_state = State::Disconnected;
			_timer = 30 * 1000;
		}
		break;
	default:
		switch (_wifi->get_connection_status()) {
		case NSAPI_STATUS_LOCAL_UP:
		case NSAPI_STATUS_GLOBAL_UP:
			if (_state == State::Connecting) {
				_owner->WifiConnected();
			}
			_state = State::Connected;
			_timer = osWaitForever;
			break;
		case NSAPI_STATUS_CONNECTING:
			_state = State::Connecting;
			_timer = 1000;
			break;
		case NSAPI_STATUS_DISCONNECTED:
		case NSAPI_STATUS_ERROR_UNSUPPORTED:
		default:
			_state = State::Disconnected;
			_timer = 30 * 1000;
			break;
		}
		break;
	}
}

NetTask::NetTask(GlobalState *globalState, ESP32Interface *wifi) :
	TaskThread(&_task, osPriorityNormal, (1024 * 8), NULL, "NetTask"),
	_tasks{ &_wifiTask, &_ntpTask, &_uploadTask, &_googleDriveTask },
	_task(_tasks, sizeof(_tasks) / sizeof(_tasks[0])),
	_globalState(globalState),
	_wifi(wifi),
	_ntpTask(this),
	_uploadTask(this),
	_wifiTask(this, wifi),
	_googleDriveTask(this),
	upload_file(NULL)
{
}

NetTask::~NetTask()
{
}

void NetTask::Init(std::string ssid, std::string password, std::string host_name,
	std::string server, std::string storage)
{
	_wifiTask.Init(ssid, password, host_name);
	_uploadTask.Init(server, storage);
}

#define CHAR3_TO_INT(a, b, c)  (((int)a) + (((int)b) << 8) + (((int)c) << 16))

bool NetTask::InvokeNtp()
{
	char temp[32];
	char mon[4], week[4];
	struct tm tm;

	if (!_wifi->ntp_time(&tm)) {
		return false;
	}

	set_time(mktime(&tm) + (9 * 60 * 60));

	return true;
}

bool NetTask::IsActive()
{
	return _uploadTask.GetState() == UploadTask::State::Update;
}

bool NetTask::QuerySever(const std::string hostname, SocketAddress &addr)
{
	return _wifi->mdns_query(hostname.c_str(), addr);
}

bool NetTask::Update(SocketAddress server, SocketAddress storage)
{
	auto url = std::string("http://") + std::string(server.get_ip_address())
		+ ":3000/update?ip=" + std::string(storage.get_ip_address());
	auto get_req = new HttpRequest(_wifi, HTTP_GET, url.c_str());
	auto get_res = get_req->send();
	return (get_res != NULL) && (get_res->get_status_code() == 200);
}

void NetTask::UploadRequest(std::string filename)
{
	_uploadTask.UploadRequest(filename);
}

bool NetTask::Upload(SocketAddress server, std::string filename)
{
	if (upload_file != NULL)
		return false;

	upload_file = fopen(filename.c_str(), "rb");

	auto url = std::string("http://") + std::string(server.get_ip_address())
		+ ":3000/upload?name=" + filename;
	auto post_req = new HttpRequest(_wifi, HTTP_POST, url.c_str());
	auto post_res = post_req->send(mbed::callback(this, &NetTask::UploadBody));

	fclose(upload_file);
	upload_file = NULL;

	if ((post_res == NULL) || (post_res->get_status_code() == 200))
		return false;
	return true;
}

size_t NetTask::UploadBody(char *buf, size_t length)
{
	return fread(buf, sizeof(char), length, upload_file);
}

void NetTask::WifiStatus(nsapi_event_t evt)
{
	printf("NetTask::WifiStatus(%d)\r\n", (int)evt);
	Signal(InterTaskSignals::WifiStatusChanged);
}

void NetTask::WifiConnected()
{
	printf("NetTask::WifiConnected\r\n");
	Signal(InterTaskSignals::WifiConnected);
}

bool NetTask::WifiSleep(bool enable)
{
	return _wifi->sleep(enable);
}
