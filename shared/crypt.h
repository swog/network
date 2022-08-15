#pragma once

void xor_crypt(
	char* dst, size_t dst_size,
	const char* src, size_t src_size,
	const char* key, size_t key_size
);