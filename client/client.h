#pragma once

// Clientside extensions
struct client_ext {
	client_ext() {
		last_recv = 0;
		last_send = 0;
	}

	time_t last_recv;
	time_t last_send;
};

class client {
public:
	friend class stream;

	~client();
	client();
	client(const char* ip, int port);

	void connect(const char* ip, int port);

	void nonblocking(bool nb);

	bool is_open() const;

	std::string addr() const;

	void close();

	const auto& ext() const {
		return _ext;
	}

	auto& ext() {
		return _ext;
	}

	// Handle input buffer
	void update();

	stream& stream() {
		return _stream;
	}

private:
	SOCKET _tcp, _udp;
	// Destination address
	// Used both by TCP and UDP
	struct sockaddr_in _addr;
	client_ext _ext;
	class stream _stream;
};

client& get_client();