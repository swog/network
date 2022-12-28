#pragma once

// The network cannot handle buffered output!
// TODO: Add an output stream buffer
// This network system could contain defined network messages with
//	an expected network packet size to slowly clear out the output buffer
// Additionally, this system would allow for packet headers,
// E.g.: A magic number.

// RCON shared configuration
#define RCON_PASS_SIZE 32
#define RCON_PASS_KEY "Cake"
#define RCON_PASS_KEY_SIZE sizeof(RCON_PASS_KEY)

// Written on server by sv_oncon in sv_main.cpp
// Received on client by svc_serverinfo_f in cl_messages.cpp
typedef struct {
	std::string name;
	std::string motd;
	size_t numclients;
	size_t maxclients;
} serverinfodata;

typedef struct {
	std::string name;
} clientinfodata;

// Client->Server packets
enum clc_message : size_t {
	clc_nop,			// Updates last received packet time
	clc_exit,			// Client has closed the connection
	clc_rcon,			// Execute command on server, requires prior auth
	clc_rcon_password,
	clc_clientinfo,
};

// Server->Client packets
enum svc_message : size_t {
	svc_nop,		// Updates last received packet time
	svc_exit,		// Server has closed the connection
	svc_exec,		// Execute command on client
	svc_print,
	svc_serverinfo,	// Server name, MOTD.
};

class net_message {
public:
	net_message() {
		_id = 0;
	}

	virtual const char* name() const = 0;
#ifdef _SERVER
	virtual void process(class server& sv, std::shared_ptr<class client> cl) = 0;
#else
	virtual void process(class client& cl) = 0;
#endif

	virtual size_t protocol() const = 0;

	// clc_*/svc_*
	const size_t& identifier() const {
		return _id;
	}

	size_t _id;
};

#ifdef _SERVER
#define NET_MESSAGE(_name, _proto) \
	class _name##_handler : public net_message { \
	public: \
		_name##_handler() { \
			_id = _name; \
		} \
		virtual const char* name() const { \
			return #_name; \
		} \
		virtual void process(class server& sv, std::shared_ptr<client> cl); \
		virtual size_t protocol() const { \
			return _proto; \
		} \
	} _name##_inst; \
	void _name##_handler::process(server& sv, std::shared_ptr<client> cl)
#else
#define NET_MESSAGE(_name, _proto) \
	class _name##_handler : public net_message { \
	public: \
		_name##_handler() { \
			_id = _name; \
		} \
		virtual const char* name() const { \
			return #_name; \
		} \
		virtual void process(client& cl); \
		virtual size_t protocol() const { \
			return _proto; \
		} \
	} _name##_inst; \
	void _name##_handler::process(client& cl)
#endif

#define NET_MESSAGE_TCP(_name) NET_MESSAGE(_name, IPPROTO_TCP)
#define NET_MESSAGE_UDP(_name) NET_MESSAGE(_name, IPPROTO_UDP)

#define DECLARE_NET_MESSAGE(_name) \
	extern class _name##_handler _name##_inst

serverinfodata& get_serverinfo();

#ifdef _SERVER
// Receive RCON command
DECLARE_NET_MESSAGE(clc_rcon);
// Receive RCON password
DECLARE_NET_MESSAGE(clc_rcon_password);

#define net_messages clc_messages

extern std::vector<net_message*> clc_messages;

class client;

// Send a NOP to keep alive
void sv_nop(std::shared_ptr<client> cl);

// Kick client, newline is included on the client's end.
void sv_kick(std::shared_ptr<client> cl, const char* reason = NULL);
// Does not print a newline character on the client, so include it.
void sv_printf(std::shared_ptr<client> cl, const char* format, ...);
#else
extern std::vector<net_message*> svc_messages;

#define net_messages svc_messages

// Send a NOP to keep alive
void cl_nop();

// Send an exit, reasons aren't allowed
void cl_exit();
#endif