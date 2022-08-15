#include "stdafx.h"

stream::~stream() {
	_so = INVALID_SOCKET;
}

stream::stream() {
	_so = INVALID_SOCKET;
}

stream::stream(SOCKET so) {
	_so = so;
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