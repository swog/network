#pragma once

class stream {
public:
	friend class client;

	~stream();
	stream();
	stream(SOCKET tcp, SOCKET udp, 
		struct sockaddr_in addr);
	
	size_t tcp_recv(void* dst, size_t size);
	void tcp_send(const void* src, size_t size);

	template<class T>
	size_t tcp_recv(T& dst) {
		return tcp_recv(&dst, sizeof(dst));
	}

	size_t tcp_recv(std::string& dst) {
		dst.clear();
		size_t num;
		char ch;
		for (size_t i = 0; i < 4096; i++) {
			num = 0;
			ch = 0;
			num = tcp_recv(ch);
			if (!num || !ch)
				break;
			dst.push_back(ch);
		}
		return dst.size();
	}

	template<class T>
	void tcp_send(T src) {
		tcp_send(&src, sizeof(src));
	}

	void tcp_send(const std::string& str) {
		tcp_send(str.data(), str.size() + 1);
	}

	template<class T>
	size_t udp_recv(T& dst) {
		return udp_recv(&dst, sizeof(dst));
	}

	size_t udp_recv(void* dst, size_t size);
	void udp_send(const void* src, size_t size);

	void udp_send(const std::string& str) {
		udp_send(str.data(), str.size() + 1);
	}

	template<class T>
	void udp_send(T src) {
		udp_send(&src, sizeof(src));
	}

	// Input streams
	void update();

#ifdef _SERVER
private:
#endif
	void tcp_flush();
	void udp_flush();

	// Flush both out streams
	void flush();

private:
	SOCKET _tcp, _udp;
	// We can use the same port
	struct sockaddr_in _addr;
	std::queue<byte> _tcp_in, _udp_in;
	std::vector<byte> _tcp_out, _udp_out;
};