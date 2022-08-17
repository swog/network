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

// Client->Server packets
enum clc_message : size_t {
	clc_nop,	// Updates last received packet time
	clc_exit,	// Client has closed the connection
	clc_rcon,	// Execute command on server, requires authentication
	clc_rcon_password, // RCON password
};

// Server->Client packets
enum svc_message : size_t {
	svc_nop,	// Updates last received packet time
	svc_exit,	// Server has closed the connection
	svc_exec,	// Execute command on client
	svc_print,	// Print message
	svc_serverinfo,	// Server information
};

typedef void (*svc_msgfn)(class client& cl, size_t cmd);
typedef void (*clc_msgfn)(class server& sv, size_t cmd, std::shared_ptr<client> cl);

serverinfodata& get_serverinfo();

#ifdef _SERVER
// Receive RCON command
void clc_rcon_f(server& sv, size_t cmd, std::shared_ptr<client> cl);
// Receive RCON password
void clc_rcon_password_f(class server& sv, size_t cmd, std::shared_ptr<client> cl);

std::vector<clc_msgfn>& clc_messages();

// Send a NOP to keep alive
void sv_nop(std::shared_ptr<client> cl);

// Kick client, newline is included on the client's end.
void sv_kick(std::shared_ptr<client> cl, const char* reason = NULL);
// Does not print a newline character on the client, so include it.
void sv_printf(std::shared_ptr<client> cl, const char* format, ...);
#else
std::vector<svc_msgfn>& svc_messages();

// Send a NOP to keep alive
void cl_nop();

// Send an exit, reasons aren't allowed
void cl_exit();
#endif