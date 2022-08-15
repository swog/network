#include "stdafx.h"
#include "net.h"

void net_send_xor(stream& s,
	const char* src, size_t size,
	const char* key, size_t key_size) {
	char* dst = new char[size];
	xor_crypt(dst, size, src, size, key, key_size);
	for (size_t i = 0; i < size; i++)
		s << dst[i];
	delete[] dst;
}

void net_recv_xor(stream& s,
	char* dst, size_t size,
	const char* key, size_t key_size) {
	for (size_t i = 0; i < size; i++)
		s >> dst[i];
	xor_crypt(dst, size, dst, size, key, key_size);
}