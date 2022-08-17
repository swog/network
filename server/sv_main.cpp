#include "stdafx.h"
#include "server.h"
#include "console.h"
#include "whitelist.h"

#define SERVER_PORT 1215
#define SERVER_MAXCON 8
#define SERVER_NAME "Example Server"
#define SERVER_MOTD "Sample MOTD\n"
#define SERVER_TIMEOUT 20

// global to get server reference
server& get_server() {
	static server sv(SERVER_PORT, SERVER_MAXCON);
	return sv;
}

console& get_console() {
	static console con;
	return con;
}

serverinfodata& get_serverinfo() {
	static serverinfodata _serverinfo {SERVER_NAME, SERVER_MOTD, 0, SERVER_MAXCON};
	_serverinfo.numclients = get_server().size();
	return _serverinfo;
}

// on connection callback for server
static void sv_oncon(server& sv, std::shared_ptr<client> cl) {
	con_printf("Client %s connected\n", cl->addr().c_str());
	
	if (!sv_inwhitelist(cl->addr().c_str())) {
		sv_kick(cl, "Not on whitelist");
		return;
	}
	
	// Send server info
	auto& svi = get_serverinfo();
	auto s = cl->stream();
	s << svc_serverinfo;
	s << svi.name;
	s << svi.motd;
	s << svi.numclients;
	s << svi.maxclients;
	s.flush();
}
 
static void options(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--whitelist"))
			sv_enablewhitelist();
	}
}

int main(int argc, char **argv) {
	options(argc, argv);
	auto& sv = get_server();
	sv.nonblocking(1);
	sv.set_onconnect(sv_oncon);
	get_console();

	int num = 0;
	size_t cmd;
	const auto& messages = clc_messages();
	time_t tim;

	while (sv.is_open()) {
		sv.update();

		for (size_t i = 0; i < sv.size(); i++) {
			auto& con = sv[i];
			auto s = con->stream();
			cmd = 0;
			num = s >> cmd;
			tim = time(NULL);
			// Send NOP if client is about to timeout
			// Send it here to ensure that it gets sent even if we havent recv in a while.
			if (tim - con->ext().last_send >= SERVER_TIMEOUT - 10)
				sv_nop(con);
			if (num <= 0) {
				// If there's no bytes read, and its timed out: kick.
				if (tim - con->ext().last_recv >= SERVER_TIMEOUT)
					sv_kick(con, "Timed out");
				continue;
			}
			// We received a byte
			con->ext().last_recv = time(NULL);
			// Number of bytes isnt the size of a command, wait...
			if (num != sizeof(cmd))
				continue;
			if (cmd >= messages.size()) {
				con_printf("Message out of bounds %llu (%i) %s\n", cmd, num, con->addr().c_str());
				continue;
			}
			messages[cmd](sv, cmd, con);
		}

		console::flush();
	}
}