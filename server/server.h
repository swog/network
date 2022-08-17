#pragma once

// Client extensions
// Extensions add variables to client network objects thru an abstraction layer.
// Note that extensions do not contain a pointer to the client object.
// Each client has an extension.
struct client_ext {
	client_ext() {
		rcon_attempts = 0;
		rcon_authed = 0;
		last_recv = 0;
		last_send = 0;
	}

	// RCON system
	size_t rcon_attempts;
	static constexpr const size_t rcon_maxattempts = 3;

	bool rcon_authed;
	time_t last_recv;	// Last packet received
	time_t last_send;
};

// Server's client instance
//	Held via a shared_ptr so that it is cleared when the server's
//		client vector destructs.
//	Some methods are inline so that they are more efficient.
class client {
public:
	friend class stream;
	friend class server;

	~client() {
		close();
	}

	client();
	client(SOCKET so, struct sockaddr_in addr);

	stream stream() {
		return {*this};
	}

	void close();

	bool is_open() const {
		return _so != INVALID_SOCKET;
	}

	std::string addr() const;

	const auto& ext() const {
		return _ext;
	}

	auto& ext() {
		return _ext;
	}

	const auto& uid() const {
		return _uid;
	}

private:
	SOCKET _so;
	struct sockaddr_in _addr;
	client_ext _ext;
	size_t _uid;		// Unique identifier.
						// We could use _so, which is unique.
						// However, I think that leaking the socket descriptor
						//	could lead to vulnerability chaining
						// So we create one based on the connection time
};

typedef void (*clc_confn)(class server& sv, std::shared_ptr<client> cl);

// Server instance object over TCP
//	The server cannot rebind to a different port.
//	Both sync and async methods are defined
// 
// Receiving a client synchronous code:
//	client cl;
//	sv >> cl;
//
// Receiving a client asynchronous code:
//	sv.nonblocking(1);
//	while (sv.is_open) {
//		sv.update();
//		for (size_t i = 0; i < sv.size(); i++) {
//			// Receive packet from client& sv[i]
//		}
//	}
class server {
public:
	~server();
	server(int port, int maxconn);

	void set_onconnect(clc_confn oncon) {
		_oncon = oncon;
	}

	void close();
	bool is_open() const {
		return _so != INVALID_SOCKET;
	}

	void update();
	void nonblocking(bool nb);

	client& operator>>(client& cli);
	// Remove client from connection list
	void remove(std::shared_ptr<client> cl);

	// Safe loop if a client was removed.
	auto begin() {
		return _cons.begin();
	}

	auto end() {
		return _cons.end();
	}

	// Backwards compatible, but also for using an index to reference a client.
	// This was used by sv_kick, and commands that take a client as an argument.
	auto size() const {
		return _cons.size();
	}

	auto& operator[](size_t i) {
		return _cons[i];
	}

	const auto& operator[](size_t i) const {
		return _cons[i];
	}

private:
	SOCKET _so;
	std::vector<std::shared_ptr<client>> _cons;
	clc_confn _oncon;
};

server& get_server();