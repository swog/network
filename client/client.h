#pragma once

class client {
public:
	~client();
	client();
	client(const char* ip, int port);

	void connect(const char* ip, int port);

	void nonblocking(bool nb);

	bool is_open() const;

	stream stream();

	std::string addr() const;

	void close();

private:
	SOCKET _so;
	struct sockaddr_in _addr;
};

client& get_client();