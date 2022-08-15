#include "stdafx.h"
#include "client.h"
#include "console.h"

serverinfodata& get_serverinfo() {
	static serverinfodata _serverinfo;
	return _serverinfo;
}

static void svc_nop_f(client& cl, size_t cmd) {
}

// close net message received from server
static void svc_exit_f(client& cl, size_t cmd) {
	std::string reason;
	cl.stream() >> reason;
	con_printf("Connection closing: %s\n", reason.c_str());
	get_console().set_open(0);
	cl.close();
}

// execute command from server
static void svc_exec_f(client& cl, size_t cmd) {
	std::string cmdstr;
	cl.stream() >> cmdstr;
	con_printf("Executing command from server: %s\n", cmdstr.c_str());
	console::exec(cmdstr.c_str());
}

// print message to console
static void svc_print_f(client& cl, size_t cmd) {
	std::string str;
	cl.stream() >> str;
	con_printf("%s", str.c_str());
}

static void svc_serverinfo_f(client& cl, size_t cmd) {
	auto& svi = get_serverinfo();
	auto s = cl.stream();
	s >> svi.name;
	s >> svi.motd;
	s >> svi.numclients;
	s >> svi.maxclients;

	con_printf("Connected to %s\n", svi.name.c_str());

	if (svi.motd.size()) {
		con_printf("MOTD: %s", svi.motd.c_str());
	}
}

std::vector<svc_msgfn>& svc_messages() {
	static std::vector<svc_msgfn> msgs = {
		svc_nop_f, svc_exit_f, svc_exec_f,
		svc_print_f, svc_serverinfo_f
	};
	return msgs;
}
