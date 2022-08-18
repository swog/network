#include "stdafx.h"
#include "client.h"
#include "console.h"
#include "messages.h"

client::~client() {
	close();
	WSACleanup();
}

client::client() {
	_tcp = _udp = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));
}

client::client(const char* ip, int port) {
	connect(ip, port);
}

// connect client to server
void client::connect(const char* ip, int port) {
	_tcp = INVALID_SOCKET;
	_udp = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));

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

	_addr.sin_family = AF_INET;
	_addr.sin_port = htons((u_short)port);
	_addr.sin_addr.s_addr = inet_addr(ip);

	if (::connect(_tcp, (const sockaddr*)&_addr, sizeof(_addr)) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (_udp == INVALID_SOCKET) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	ext().last_recv = ext().last_send = time(NULL);
	_stream = class stream(_tcp, _udp, _addr);
}

void client::nonblocking(bool nb) {
	u_long mode = nb;
	ioctlsocket(_tcp, FIONBIO, &mode);
	mode = nb;
	ioctlsocket(_udp, FIONBIO, &mode);
}

bool client::is_open() const {
	return _tcp != INVALID_SOCKET && _udp != INVALID_SOCKET;
}

std::string client::addr() const {
	std::string str = inet_ntoa(_addr.sin_addr);
	str += ':';
	str += std::to_string((unsigned int)ntohs(_addr.sin_port));
	return str;
}

void client::close() {
	if (_tcp != INVALID_SOCKET)
		closesocket(_tcp);
	if (_udp != INVALID_SOCKET)
		closesocket(_udp);
	_tcp = _udp = INVALID_SOCKET;
}

#define SERVER_TIMEOUT 20

// Handle input buffer
void client::update() {
	if (!is_open())
		return;
	_stream.update();
	size_t cmd = 0, num = 0;
	num = _stream.tcp_recv(cmd);
	time_t tim = time(NULL);
	const auto& messages = svc_messages;
	net_message* msg;
	if (tim - ext().last_send >= SERVER_TIMEOUT - 10)
		cl_nop();
	if (num == sizeof(cmd)) {
		ext().last_recv = tim;
		if (cmd >= messages.size())
			con_printf("TCP message out of bounds\n");
		else {
			msg = messages[cmd];
			if (msg->protocol() != IPPROTO_TCP)
				con_printf("Server message from TCP is not TCP\n");
			else
				msg->process(*this);
		}
	}
	cmd = 0;
	num = _stream.udp_recv(cmd);
	if (num == sizeof(cmd)) {
		ext().last_recv = tim;
		if (num >= messages.size())
			con_printf("UDP message out of bounds\n");
		else {
			msg = messages[cmd];
			if (msg->protocol() != IPPROTO_UDP)
				con_printf("Server message from UDP is not UDP\n");
			else
				msg->process(*this);
		}
	}
	if (tim - ext().last_recv >= SERVER_TIMEOUT) {
		cl_exit();
		con_printf("Server timed out\n");
	}
}
