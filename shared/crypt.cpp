#include "stdafx.h"
#include "crypt.h"

void xor_crypt(
	char* dst, size_t dst_size,
	const char* src, size_t src_size,
	const char* key, size_t key_size
) {
	size_t size = min(dst_size, src_size);
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i] ^ key[i % (key_size - 1)];
}
