#include "stdafx.h"
#include "server.h"
#include "whitelist.h"
#include "messages.h"

server::~server() {
	for (auto& cl : _cons)
		sv_kick(cl, "Server shutting down");

	_cons.clear();

	close();
	WSACleanup();
}

server::server(int port, int maxconn) {
	_tcp = _udp =  INVALID_SOCKET;
	_oncon = NULL;

	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		return;
	}

	_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_tcp == INVALID_SOCKET) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		return;
	}

	_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (_udp == INVALID_SOCKET) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in sin;
	ZeroMemory(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short)port);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(_tcp, (const sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	if (listen(_tcp, maxconn) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	if (bind(_udp, (const sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}
}

void server::close() {
	if (_tcp != INVALID_SOCKET)
		closesocket(_tcp);
	if (_udp != INVALID_SOCKET)
		closesocket(_udp);
	_tcp = _udp = INVALID_SOCKET;
}

#define SERVER_TIMEOUT 20

void server::update() {
	if (!is_open())
		return;
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET so = accept(_tcp, (sockaddr*)&addr, &addrlen);
	if (so != INVALID_SOCKET) {
		std::shared_ptr<client> con(new client(so, addr));
		u_long mode = 1;
		ioctlsocket(so, FIONBIO, &mode);
		_cons.push_back(con);
		if (_oncon)
			_oncon(*this, con);
	}
	const auto& messages = clc_messages;
	time_t tim = time(NULL);
	size_t cmd = 0, num = 0;
	for (size_t i = 0; i < size(); i++) {
		cmd = num = 0;
		auto cl = _cons[i];
		cl->update();
		auto& s = cl->stream();
		num = s.tcp_recv(cmd);
		net_message* msg;
		if (tim - cl->ext().last_send >= SERVER_TIMEOUT - 10)
			sv_nop(cl);
		if (num == sizeof(cmd)) {
			cl->ext().last_recv = tim;
			if (cmd >= messages.size())
				continue;
			else {
				msg = messages[cmd];
				if (msg->protocol() != IPPROTO_TCP)
					continue;
				else
					msg->process(*this, cl);
			}
		}
		cmd = 0;
		num = s.udp_recv(cmd);
		if (num == sizeof(cmd)) {
			cl->ext().last_recv = tim;
			if (num >= messages.size())
				continue;
			else {
				msg = messages[cmd];
				if (msg->protocol() != IPPROTO_UDP)
					continue;
				else
					msg->process(*this, cl);
			}
		}
		if (tim - cl->ext().last_recv >= SERVER_TIMEOUT)
			sv_kick(cl, "Timed out");
	}
}

void server::nonblocking(bool nb) {
	u_long mode = nb;
	ioctlsocket(_tcp, FIONBIO, &mode);
	mode = nb;
	ioctlsocket(_udp, FIONBIO, &mode);
}

void server::remove(std::shared_ptr<client> cl) {
	_cons.erase(std::remove(_cons.begin(), _cons.end(), cl));
}

client::client() {
	_tcp = _udp = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));
	_uid = 0;
}

client::client(SOCKET so, sockaddr_in addr) {
	_tcp = so;
	_udp = get_server()._udp;
	_addr = addr;
	auto tim = time(NULL);
	ext().last_recv = ext().last_send = tim;
	_uid = -1 ^ tim;
	_stream = class stream(so, _udp, addr);
}

void client::close() {
	if (_tcp != INVALID_SOCKET)
		closesocket(_tcp);
	if (_udp != INVALID_SOCKET)
		closesocket(_udp);
	_tcp = _udp = INVALID_SOCKET;
	_uid = 0;
	ZeroMemory(&_addr, sizeof(_addr));
}

std::string client::addr() const {
	std::string name = inet_ntoa(_addr.sin_addr);
	name += ':';
	name += std::to_string((unsigned int)ntohs(_addr.sin_port));
	return name;
}