#include "stdafx.h"
#include "console.h"
#include "server.h"
#include "messages.h"

CONSOLE_COMMAND(exec, "Execute command on client", 0) {
	if (args.argv.size() < 3) {
		con_printf("Usage: exec <index> \"<command>\"");
		return;
	}
	auto& sv = get_server();
	size_t index = (size_t)atoi(args[1].c_str());
	if (index >= sv.size()) {
		con_printf("kick: incorrect index\n");
		return;
	}
	auto& cl = sv[index];
	auto& s = cl->stream();
	s.tcp_send(svc_exec);
	s.tcp_send(args[2]);
	cl->tcp_flush(s);
}

// Exit command to close server
CONSOLE_COMMAND(exit, "Close program", 0) {
	get_server().close();
	get_console().set_open(0);
}

CONSOLE_COMMAND(kick, "Remove client from server with an optional reason", 0) {
	if (args.size() < 2) {
		con_printf("Usage: kick <id> [reason]\n");
		return;
	}
	auto& sv = get_server();
	size_t index = (size_t)atoi(args[1].c_str());
	if (index >= sv.size() || !sv[index]->is_open()) {
		con_printf("kick: incorrect index\n");
		return;
	}
	sv_kick(sv[index], args.size() > 2 ? args[2].c_str() : NULL);
}

CONSOLE_COMMAND(status, "List active clients", 0) {
	auto& sv = get_server();
	con_printf("Clients:\n");
	for (size_t i = 0; i < sv.size(); i++) {
		auto& cl = sv[i];
		if (cl->is_open())
			con_printf("%llu: %s\n", i, cl->addr().c_str());
	}
}