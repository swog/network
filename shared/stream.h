#pragma once

class stream {
public:
	~stream();
	stream(class client& cl);

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

	void operator<<(const std::string& str);
	void operator<<(const char* str);

	void flush();

private:
	SOCKET _so;
	std::vector<unsigned char> _outbuf;
	client& _cl;
};