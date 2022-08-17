#include "stdafx.h"
#ifdef _SERVER
#include "server.h"
#else
#include "client.h"
#endif

stream::~stream() {
	_so = INVALID_SOCKET;
}

stream::stream(client& cl) : _cl(cl) {
	_so = cl._so;
}

int stream::send(const void* src, size_t size) {
	return ::send(_so, (const char*)src, (int)size, 0);
}

int stream::recv(void* dst, size_t size) {
	return ::recv(_so, (char*)dst, (int)size, 0);
}

int stream::operator>>(std::string& str) {
	char ch;
	int num;
	while (str.size() < 63) {
		ch = 0;
		num = recv(&ch, sizeof(ch));
		if (num != sizeof(ch) || !ch)
			break;
		str += ch;
	}
	return (int)str.size();
}

void stream::operator<<(const std::string& str) {
	for (const auto& ch : str)
		_outbuf.push_back(ch);
	_outbuf.push_back(0);
}

void stream::operator<<(const char* str) {
	size_t size = strlen(str) + 1;
	for (size_t i = 0; i < size; i++)
		_outbuf.push_back(str[i]);
}

void stream::flush() {
	send(_outbuf.data(), _outbuf.size());
	_outbuf.clear();

	// Shared magic
	_cl.ext().last_send = time(NULL);
}
