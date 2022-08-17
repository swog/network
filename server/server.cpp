#include "stdafx.h"
#include "server.h"
#include "whitelist.h"

server::~server() {
	for (auto& cl : _cons) {
		sv_kick(cl, "Server shutting down");
	}

	_cons.clear();

	close();
	WSACleanup();
}

server::server(int port, int maxconn) {
	_so = INVALID_SOCKET;
	_oncon = NULL;

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

	struct sockaddr_in sin;
	ZeroMemory(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short)port);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(_so, (const sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}

	if (listen(_so, maxconn) == SOCKET_ERROR) {
		fprintf(stderr, "Socket error %i\n", WSAGetLastError());
		close();
		return;
	}
}

void server::close() {
	if (_so != INVALID_SOCKET)
		closesocket(_so);
	_so = INVALID_SOCKET;
}

void server::update() {
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	SOCKET so = accept(_so, (sockaddr*)&addr, &addrlen);
	if (so != INVALID_SOCKET) {
		std::shared_ptr<client> con(new client(so, addr));
		u_long mode = 1;
		ioctlsocket(so, FIONBIO, &mode);
		_cons.push_back(con);
		if (_oncon)
			_oncon(*this, con);
	}
}

void server::nonblocking(bool nb) {
	u_long mode = nb;
	ioctlsocket(_so, FIONBIO, &mode);
}

client& server::operator>>(client& cli) {
	int addrlen = sizeof(cli._addr);
	cli._so = accept(_so, (sockaddr*)&cli._addr, &addrlen);
	return cli;
}

void server::remove(std::shared_ptr<client> cl) {
	_cons.erase(std::remove(_cons.begin(), _cons.end(), cl));
}

client::client() {
	_so = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));
	_uid = 0;
}

client::client(SOCKET so, sockaddr_in addr) {
	_so = so;
	_addr = addr;
	auto tim = time(NULL);
	ext().last_recv = ext().last_send = tim;
	_uid = -1 ^ tim;
}

void client::close() {
	if (_so != INVALID_SOCKET)
		closesocket(_so);
	_so = INVALID_SOCKET;
	_uid = 0;
	ZeroMemory(&_addr, sizeof(_addr));
}

std::string client::addr() const {
	std::string name = inet_ntoa(_addr.sin_addr);
	name += ':';
	name += std::to_string((unsigned int)ntohs(_addr.sin_port));
	return name;
}