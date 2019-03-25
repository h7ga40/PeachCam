/* Socket implementation of NetworkInterfaceAPI
 * Copyright (c) 2017 Renesas Electronics Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "SocketInterface.h"

// SocketInterface implementation
SocketInterface::SocketInterface() :
	SocketStack(),
	_dhcp(true),
	_ap_ssid(),
	_ap_pass(),
	_ap_sec(NSAPI_SECURITY_NONE),
	_ip_address(),
	_netmask(),
	_gateway(),
	_connection_status(NSAPI_STATUS_DISCONNECTED),
	_connection_status_cb(NULL)
{
	//_esp->attach_wifi_status(callback(this, &SocketInterface::wifi_status_cb));
}

nsapi_error_t SocketInterface::set_network(const char *ip_address, const char *netmask, const char *gateway)
{
	_dhcp = false;

	strncpy(_ip_address, ip_address ? ip_address : "", sizeof(_ip_address));
	_ip_address[sizeof(_ip_address) - 1] = '\0';
	strncpy(_netmask, netmask ? netmask : "", sizeof(_netmask));
	_netmask[sizeof(_netmask) - 1] = '\0';
	strncpy(_gateway, gateway ? gateway : "", sizeof(_gateway));
	_gateway[sizeof(_gateway) - 1] = '\0';

	return NSAPI_ERROR_OK;
}

nsapi_error_t SocketInterface::set_dhcp(bool dhcp)
{
	_dhcp = dhcp;

	return NSAPI_ERROR_OK;
}

int SocketInterface::connect(const char *ssid, const char *pass, nsapi_security_t security,
	uint8_t channel)
{
	if (channel != 0) {
		return NSAPI_ERROR_UNSUPPORTED;
	}

	set_credentials(ssid, pass, security);
	return connect();
}

int SocketInterface::connect()
{
	if (!false/*_esp->dhcp(_dhcp, 1)*/) {
		return NSAPI_ERROR_DHCP_FAILURE;
	}

	if (!_dhcp) {
		if (!false/*_esp->set_network(_ip_address, _netmask, _gateway)*/) {
			return NSAPI_ERROR_DEVICE_ERROR;
		}
	}

	set_connection_status(NSAPI_STATUS_CONNECTING);
	if (!false/*_esp->connect(_ap_ssid, _ap_pass)*/) {
		set_connection_status(NSAPI_STATUS_DISCONNECTED);
		return NSAPI_ERROR_NO_CONNECTION;
	}

	return NSAPI_ERROR_OK;
}

int SocketInterface::set_credentials(const char *ssid, const char *pass, nsapi_security_t security)
{
	memset(_ap_ssid, 0, sizeof(_ap_ssid));
	strncpy(_ap_ssid, ssid, sizeof(_ap_ssid));

	memset(_ap_pass, 0, sizeof(_ap_pass));
	strncpy(_ap_pass, pass, sizeof(_ap_pass));

	_ap_sec = security;

	return 0;
}

int SocketInterface::set_channel(uint8_t channel)
{
	return NSAPI_ERROR_UNSUPPORTED;
}

int SocketInterface::disconnect()
{
	if (!false/*_esp->disconnect()*/) {
		return NSAPI_ERROR_DEVICE_ERROR;
	}

	return NSAPI_ERROR_OK;
}

const char *SocketInterface::get_ip_address()
{
	return "127.0.0.1"/*_esp->getIPAddress()*/;
}

const char *SocketInterface::get_mac_address()
{
	return "12:34:56:78:9A:BC"/*_esp->getMACAddress()*/;
}

const char *SocketInterface::get_gateway()
{
	return "127.0.0.1"/*_esp->getGateway()*/;
}

const char *SocketInterface::get_netmask()
{
	return "127.0.0.255"/*_esp->getNetmask()*/;
}

int8_t SocketInterface::get_rssi()
{
	return 127/*_esp->getRSSI()*/;
}

int SocketInterface::scan(WiFiAccessPoint *res, unsigned count)
{
	return 0/*_esp->scan(res, count)*/;
}

void SocketInterface::attach(mbed::Callback<void(nsapi_event_t, intptr_t)> status_cb)
{
	_connection_status_cb = status_cb;
}

nsapi_connection_status_t SocketInterface::get_connection_status() const
{
	return _connection_status;
}

void SocketInterface::set_connection_status(nsapi_connection_status_t connection_status)
{
	if (_connection_status != connection_status) {
		_connection_status = connection_status;
		if (_connection_status_cb) {
			_connection_status_cb(NSAPI_EVENT_CONNECTION_STATUS_CHANGE, _connection_status);
		}
	}
}

void SocketInterface::wifi_status_cb(int8_t wifi_status)
{
	/*switch (wifi_status) {
	case Socket::STATUS_DISCONNECTED:
		set_connection_status(NSAPI_STATUS_DISCONNECTED);
		break;
	case Socket::STATUS_GOT_IP:
		set_connection_status(NSAPI_STATUS_GLOBAL_UP);
		break;
	case Socket::STATUS_CONNECTED:
	default:
		// do nothing
		break;
	}*/
}

bool SocketInterface::ntp(bool enabled, int timezone, const char *server0, const char *server1, const char *server2)
{
	return false;// _esp->ntp(enabled, timezone, server0, server1, server2);
}

bool SocketInterface::ntp_time(struct tm *tm)
{
	return false;// _esp->ntp_time(tm);
}

bool SocketInterface::esp_time(struct timeval &tv)
{
	return false;// _esp->esp_time(tv);
}

int SocketInterface::ping(const char *addr)
{
	return false;// _esp->ping(addr);
}

bool SocketInterface::mdns(bool enabled, const char *hostname, const char *service, uint16_t portno)
{
	return false;// _esp->mdns(enabled, hostname, service, portno);
}

bool SocketInterface::mdns_query(const char *hostname, SocketAddress &addr)
{
	return false;// _esp->mdns_query(hostname, addr);
}

bool SocketInterface::sleep(bool enebled)
{
	return true;
}
