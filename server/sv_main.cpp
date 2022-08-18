#include "stdafx.h"
#include "server.h"
#include "console.h"
#include "whitelist.h"
#include "messages.h"

#define SERVER_PORT 1215
#define SERVER_MAXCON 8
#define SERVER_NAME "Example Server"
#define SERVER_MOTD "Sample MOTD\n"

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
	auto& s = cl->stream();
	s.tcp_send(svc_serverinfo);
	s.tcp_send(svi.name);
	s.tcp_send(svi.motd);
	s.tcp_send(svi.numclients);
	s.tcp_send(svi.maxclients);
	cl->tcp_flush(s);
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

	const auto& messages = clc_messages;

	while (get_console().is_open()) {
		console::flush();
		sv.update();
	}
}