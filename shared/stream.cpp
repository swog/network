#include "stdafx.h"
#ifdef _SERVER
#include "server.h"
#else
#include "client.h"
#endif

stream::~stream() {
	flush();
	_tcp = _udp = INVALID_SOCKET;
}

stream::stream() {
	_tcp = _udp = INVALID_SOCKET;
	ZeroMemory(&_addr, sizeof(_addr));
}

stream::stream(SOCKET tcp, SOCKET udp, struct sockaddr_in addr) {
	_tcp = tcp;
	_udp = udp;
	_addr = addr;
}

size_t stream::tcp_recv(void* dst, size_t size) {
	size_t sz = min(_tcp_in.size(), size);
	for (size_t i = 0; i < sz; i++) {
		((byte*)dst)[i] = _tcp_in.front();
		_tcp_in.pop();
	}
	return sz;
}

void stream::tcp_send(const void* src, size_t size) {
	for (size_t i = 0; i < size; i++)
		_tcp_out.push_back(((byte*)src)[i]);
}

void stream::tcp_flush() {
	send(_tcp, (char*)_tcp_out.data(), (int)_tcp_out.size(), 0);
	_tcp_out.clear();
#ifndef _SERVER
	get_client().ext().last_send = time(NULL);
#endif
}

size_t stream::udp_recv(void* dst, size_t size) {
	size_t sz = min(_udp_in.size(), size);
	for (size_t i = 0; i < sz; i++) {
		((byte*)dst)[i] = _udp_in.front();
		_udp_in.pop();
	}
	return sz;
}

void stream::udp_send(const void* src, size_t size) {
	for (size_t i = 0; i < size; i++)
		_udp_out.push_back(((byte*)src)[i]);
}

// Input streams
void stream::update() {
	char val = 0;
	int num = recv(_tcp, &val, sizeof(val), 0);
	while (num > 0) {
		_tcp_in.push(val);
		num = recv(_tcp, &val, sizeof(val), 0);
	}
	val = 0;
	int fromlen = sizeof(_addr);
	num = recvfrom(_udp, &val, sizeof(val), 0, (sockaddr*)&_addr, &fromlen);
	while (num > 0) {
		_udp_in.push(val);
		num = recvfrom(_udp, &val, sizeof(val), 0, (sockaddr*)&_addr, &fromlen);
	}
}

void stream::udp_flush() {
	sendto(_udp, (char*)_udp_out.data(), (int)_udp_out.size(), 0,
		(sockaddr*)&_addr, sizeof(_addr));
	_udp_out.clear();
#ifndef _SERVER
	get_client().ext().last_send = time(NULL);
#endif
}

void stream::flush() {
	tcp_flush();
	udp_flush();
}