#include "stdafx.h"
#include "client.h"
#include "console.h"
#include "messages.h"

serverinfodata& get_serverinfo() {
	static serverinfodata _serverinfo;
	return _serverinfo;
}

NET_MESSAGE_TCP(svc_nop) {
}

// close net message received from server
NET_MESSAGE_TCP(svc_exit) {
	std::string reason;
	cl.stream().tcp_recv(reason);
	con_printf("Connection closing: %s\n", reason.c_str());
	cl.close();
}

// execute command from server
NET_MESSAGE_TCP(svc_exec) {
	std::string cmdstr;
	cl.stream().tcp_recv(cmdstr);
	console::exec(cmdstr.c_str());
}

// print message to console
NET_MESSAGE_TCP(svc_print) {
	std::string str;
	cl.stream().tcp_recv(str);
	con_printf("%s", str.c_str());
}

NET_MESSAGE_TCP(svc_serverinfo) {
	auto& svi = get_serverinfo();
	auto& s = cl.stream();
	s.tcp_recv(svi.name);
	s.tcp_recv(svi.motd);
	s.tcp_recv(svi.numclients);
	s.tcp_recv(svi.maxclients);

	con_printf("Connected to %s\n", svi.name.c_str());

	if (svi.motd.size()) {
		con_printf("MOTD: %s", svi.motd.c_str());
	}
}

std::vector<net_message*> svc_messages = {
	&svc_nop_inst, &svc_exit_inst, &svc_exec_inst,
	&svc_print_inst, &svc_serverinfo_inst
};

// Send a NOP to keep alive
void cl_nop() {
	auto& cl = get_client();
	auto& s = cl.stream();
	s.tcp_send(clc_nop);
	s.tcp_flush();
}

// Send an exit, reasons aren't allowed
void cl_exit() {
	auto& cl = get_client();
	auto& s = cl.stream();
	s.tcp_send(clc_exit);
	s.tcp_flush();
	cl.close();
}
