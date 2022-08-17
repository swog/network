#include "stdafx.h"
#include "net.h"

void net_send_xor(stream& s,
	const char* src, size_t size,
	const char* key, size_t key_size) {
	char* dst = new char[size];
	xor_crypt(dst, size, src, size, key, key_size);
	s.tcp_send(dst, size);
	delete[] dst;
}

void net_recv_xor(stream& s,
	char* dst, size_t size,
	const char* key, size_t key_size) {
	s.tcp_recv(dst, size);
	xor_crypt(dst, size, dst, size, key, key_size);
}