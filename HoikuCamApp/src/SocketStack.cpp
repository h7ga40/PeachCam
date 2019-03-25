/* Socket implementation of NetworkInterfaceAPI
 * Copyright (c) 2015 ARM Limited
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
#include "SocketStack.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define INVALID_SOCKET -1

// SocketStack implementation
SocketStack::SocketStack()
{
}

struct socket_info_t {
	int id;
	nsapi_protocol_t proto;
	bool connected;
	SocketAddress laddr;
	SocketAddress raddr;
	int keepalive; // TCP
	bool accept_id;
	bool tcp_server;
};

int SocketStack::socket_open(void **handle, nsapi_protocol_t proto)
{
	struct socket_info_t *socket = new struct socket_info_t;
	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	socket->id = INVALID_SOCKET;
	socket->proto = proto;
	socket->connected = false;
	socket->keepalive = 0;
	socket->accept_id = false;
	socket->tcp_server = false;
	*handle = socket;
	return 0;
}

int SocketStack::socket_close(void *handle)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;
	int err = 0;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	if (!close(socket->id)) {
		err = NSAPI_ERROR_DEVICE_ERROR;
	}

	delete socket;
	return err;
}

int SocketStack::socket_bind(void *handle, const SocketAddress &address)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	if (socket->id == INVALID_SOCKET) {
		return NSAPI_ERROR_PARAMETER;
	}

	int protocol;
	switch (socket->proto) {
	case NSAPI_TCP:
		protocol = SOCK_STREAM;
		break;
	case NSAPI_UDP:
		protocol = SOCK_DGRAM;
		break;
	default:
		return NSAPI_ERROR_PARAMETER;
	}

	switch (address.get_addr().version) {
	case NSAPI_IPv4:
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = address.get_port();
		memcpy(&addr.sin_addr, address.get_ip_bytes(), sizeof(addr.sin_addr));
		memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

		socket->id = ::socket(AF_INET, protocol, IPPROTO_IP);
		if (socket->id == INVALID_SOCKET)
			return NSAPI_ERROR_UNSUPPORTED;
		if (bind(socket->id, (sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
			return NSAPI_ERROR_UNSUPPORTED;
		break;
	}
	case NSAPI_IPv6:
	{
		struct sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		addr.sin6_port = address.get_port();
		memcpy(&addr.sin6_addr, address.get_ip_bytes(), sizeof(addr.sin6_addr));

		socket->id = ::socket(AF_INET6, protocol, IPPROTO_IP);
		if (socket->id == INVALID_SOCKET)
			return NSAPI_ERROR_UNSUPPORTED;
		if (bind(socket->id, (sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
			return NSAPI_ERROR_UNSUPPORTED;
		break;
	}
	case NSAPI_UNSPEC:
	default:
		return NSAPI_ERROR_UNSUPPORTED;
	}

	socket->laddr = address;
	return 0;
}

int SocketStack::socket_listen(void *handle, int backlog)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	(void)backlog;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	if (socket->proto != NSAPI_TCP) {
		return NSAPI_ERROR_UNSUPPORTED;
	}

	if (!listen(socket->id, false)) {
		return NSAPI_ERROR_DEVICE_ERROR;
	}

	socket->tcp_server = true;
	return 0;
}

int SocketStack::socket_connect(void *handle, const SocketAddress &address)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	switch (address.get_addr().version) {
	case NSAPI_IPv4:
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = address.get_port();
		memcpy(&addr.sin_addr, address.get_ip_bytes(), sizeof(addr.sin_addr));
		memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

		if (connect(socket->id, (sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
			return NSAPI_ERROR_UNSUPPORTED;
		break;
	}
	case NSAPI_IPv6:
	{
		struct sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		addr.sin6_port = address.get_port();
		memcpy(&addr.sin6_addr, address.get_ip_bytes(), sizeof(addr.sin6_addr));

		if (connect(socket->id, (sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
			return NSAPI_ERROR_UNSUPPORTED;
		break;
	}
	case NSAPI_UNSPEC:
	default:
		return NSAPI_ERROR_UNSUPPORTED;
	}

	socket->connected = true;
	return 0;
}

int SocketStack::socket_accept(void *server, void **socket, SocketAddress *address)
{
	struct socket_info_t *socket_server = (struct socket_info_t *)server;
	struct socket_info_t *socket_new = new struct socket_info_t;
	int id;

	if (!socket_new) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	struct sockaddr addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	memset(&addr, 0, sizeof(addr));
	id = accept(socket_server->id, (struct sockaddr *)&addr, &addrlen);
	if (id == INVALID_SOCKET) {
		delete socket_new;
		return NSAPI_ERROR_NO_SOCKET;
	}

	address = &socket_new->raddr;
	switch (addr.sa_family) {
	case AF_INET:
	{
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
		address->set_port(addr4->sin_port);
		address->set_ip_bytes(&addr4->sin_addr, NSAPI_IPv4);
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
		address->set_port(addr6->sin6_port);
		address->set_ip_bytes(&addr6->sin6_addr, NSAPI_IPv6);
		break;
	}
	default:
		delete socket_new;
		return NSAPI_ERROR_NO_SOCKET;
	}

	socket_new->id = id;
	socket_new->proto = NSAPI_TCP;
	socket_new->connected = true;
	socket_new->accept_id = true;
	socket_new->tcp_server = false;
	*socket = socket_new;

	return 0;
}

int SocketStack::socket_send(void *handle, const void *data, unsigned size)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	if (!send(socket->id, (char *)data, size, 0)) {
		return NSAPI_ERROR_DEVICE_ERROR;
	}

	return size;
}

int SocketStack::socket_recv(void *handle, void *data, unsigned size)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	int32_t recv = ::recv(socket->id, (char *)data, size, 0);
	if (recv == -1) {
		return NSAPI_ERROR_WOULD_BLOCK;
	}
	else if (recv < 0) {
		return NSAPI_ERROR_NO_SOCKET;
	}
	else {
	 // do nothing
	}

	return recv;
}

int SocketStack::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	switch (address.get_addr().version) {
	case NSAPI_IPv4:
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = address.get_port();
		memcpy(&addr.sin_addr, address.get_ip_bytes(), sizeof(addr.sin_addr));
		memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

		return sendto(socket->id, (char *)data, size, 0, (sockaddr *)&addr, sizeof(struct sockaddr_in));
		break;
	}
	case NSAPI_IPv6:
	{
		struct sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		addr.sin6_port = address.get_port();
		memcpy(&addr.sin6_addr, address.get_ip_bytes(), sizeof(addr.sin6_addr));

		return sendto(socket->id, (char *)data, size, 0, (sockaddr *)&addr, sizeof(struct sockaddr_in));
			return NSAPI_ERROR_UNSUPPORTED;
		break;
	}
	case NSAPI_UNSPEC:
	default:
		return NSAPI_ERROR_UNSUPPORTED;
	}
}

int SocketStack::socket_recvfrom(void *handle, SocketAddress *address, void *data, unsigned size)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	struct sockaddr addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	memset(&addr, 0, sizeof(addr));

	int ret = recvfrom(socket->id, (char *)data, size, 0, (struct sockaddr *)&addr, &addrlen);
	if (ret >= 0 && address) {
		address = &socket->raddr;
		switch (addr.sa_family) {
		case AF_INET:
		{
			struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
			address->set_port(addr4->sin_port);
			address->set_ip_bytes(&addr4->sin_addr, NSAPI_IPv4);
			break;
		}
		case AF_INET6:
		{
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
			address->set_port(addr6->sin6_port);
			address->set_ip_bytes(&addr6->sin6_addr, NSAPI_IPv6);
			break;
		}
		default:
			return NSAPI_ERROR_NO_SOCKET;
		}
	}

	return ret;
}

void SocketStack::socket_attach(void *handle, void(*callback)(void *), void *data)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!socket) {
		return;
	}

	socket_attach((void *)socket->id, callback, data);
}

nsapi_error_t SocketStack::setsockopt(nsapi_socket_t handle, int level,
	int optname, const void *optval, unsigned optlen)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!optlen) {
		return NSAPI_ERROR_PARAMETER;
	}
	else if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	switch (optname) {
	case NSAPI_KEEPALIVE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, SOL_SOCKET, SO_KEEPALIVE, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_REUSEADDR:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, SOL_SOCKET, SO_REUSEADDR, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_KEEPIDLE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, IPPROTO_TCP, TCP_KEEPIDLE, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_KEEPINTVL:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, IPPROTO_TCP, TCP_KEEPINTVL, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_LINGER:
	case NSAPI_SNDBUF:
	case NSAPI_RCVBUF:
		break;
	case NSAPI_ADD_MEMBERSHIP:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_UDP) {
			if (::setsockopt(socket->id, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_DROP_MEMBERSHIP:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_UDP) {
			if (::setsockopt(socket->id, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_NODELAY:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, IPPROTO_TCP, TCP_NODELAY, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_SO_KEEPALIVE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, SOL_SOCKET, SO_KEEPALIVE, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_SO_RCVTIMEO:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::setsockopt(socket->id, SOL_SOCKET, SO_RCVTIMEO, (char *)optval, optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	default:
		break;
	}

	return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SocketStack::getsockopt(nsapi_socket_t handle, int level,
	int optname, void *optval, unsigned *optlen)
{
	struct socket_info_t *socket = (struct socket_info_t *)handle;

	if (!optval || !optlen) {
		return NSAPI_ERROR_PARAMETER;
	}
	else if (!socket) {
		return NSAPI_ERROR_NO_SOCKET;
	}

	switch (optname) {
	case NSAPI_KEEPALIVE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, SOL_SOCKET, SO_KEEPALIVE, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_REUSEADDR:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, SOL_SOCKET, SO_REUSEADDR, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_KEEPIDLE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, IPPROTO_TCP, TCP_KEEPIDLE, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_KEEPINTVL:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, IPPROTO_TCP, TCP_KEEPINTVL, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_LINGER:
	case NSAPI_SNDBUF:
	case NSAPI_RCVBUF:
		break;
	case NSAPI_NODELAY:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, IPPROTO_TCP, TCP_NODELAY, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_ERROR:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, SOL_SOCKET, SO_ERROR, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_SO_KEEPALIVE:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, SOL_SOCKET, SO_KEEPALIVE, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	case NSAPI_SO_RCVTIMEO:
		if (level == NSAPI_SOCKET && socket->proto == NSAPI_TCP) {
			if (::getsockopt(socket->id, SOL_SOCKET, SO_RCVTIMEO, (char *)optval, (socklen_t *)optlen)) {
				return NSAPI_ERROR_OK;
			}
		}
		break;
	default:
		break;
	}

	return NSAPI_ERROR_UNSUPPORTED;
}

