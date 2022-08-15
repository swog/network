#pragma once

// Sends xor'd data
void net_send_xor(stream& s,
	const char* src, size_t size,
	const char* key, size_t key_size
);

void net_recv_xor(stream& s,
	char* dst, size_t size,
	const char* key, size_t key_size);