#include "stdafx.h"
#include "client.h"

client::~client() {
	close();
	WSACleanup();
}

client::client() {
	_so = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));
}

client::client(const char* ip, int port) {
	connect(ip, port);
}

// connect client to server
void client::connect(const char* ip, int port) {
	_so = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));

	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		return;
	}

	_so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_so == INVALID_SOCKET) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		return;
	}

	_addr.sin_family = AF_INET;
	_addr.sin_port = htons((u_short)port);
	_addr.sin_addr.s_addr = inet_addr(ip);

	if (::connect(_so, (const sockaddr*)&_addr, sizeof(_addr)) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	ext().last_recv = ext().last_send = time(NULL);
}

void client::nonblocking(bool nb) {
	u_long mode = nb;
	ioctlsocket(_so, FIONBIO, &mode);
}

bool client::is_open() const {
	return _so != INVALID_SOCKET;
}

stream client::stream() {
	return {*this};
}

std::string client::addr() const {
	std::string str = inet_ntoa(_addr.sin_addr);
	str += ':';
	str += std::to_string((unsigned int)ntohs(_addr.sin_port));
	return str;
}

void client::close() {
	if (_so != INVALID_SOCKET)
		closesocket(_so);
	_so = INVALID_SOCKET;
}
