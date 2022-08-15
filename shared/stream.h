#pragma once

class stream {
public:
	~stream();
	stream();
	stream(SOCKET so);

	int send(const void* src, size_t size);
	int recv(void* dst, size_t size);

	template<class T>
	int operator>>(T& val) {
		return recv(&val, sizeof(val));
	}

	int operator>>(std::string& str);

	template<class T>
	void operator<<(const T& val) {
		for (size_t i = 0; i < sizeof(val); i++)
			_outbuf.push_back(((const byte*)&val)[i]);
	}

	void operator<<(const std::string& str) {
		for (const auto& ch : str)
			_outbuf.push_back(ch);
		_outbuf.push_back(0);
	}

	void operator<<(const char* str) {
		size_t size = strlen(str) + 1;
		for (size_t i = 0; i < size; i++)
			_outbuf.push_back(str[i]);
	}

	void flush() {
		send(_outbuf.data(), _outbuf.size());
		_outbuf.clear();
	}

private:
	SOCKET _so;
	std::vector<unsigned char> _outbuf;
};