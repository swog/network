#include "stdafx.h"
#include "console.h"
#include "server.h"
#include "net.h"

CONSOLE_VAR_CHANGE(rcon_password, "Set remote console password", fcommand_hidevalue, "") {
	// Clear all authed players if the password changed
	auto& sv = get_server();
	for (size_t i = 0; i < sv.size(); i++) {
		if (sv[i]->ext().rcon_authed)
			sv_printf(sv[i], "RCON unauthed\n");
		sv[i]->ext().rcon_authed = 0;
	}
	sv_con_unlog();
}

// Received rcon command from client
NET_MESSAGE_TCP(clc_rcon) {
	std::string cmdstr;
	cl->stream().tcp_recv(cmdstr);
	// Make sure authenticated
	if (!cl->ext().rcon_authed)
		return;
	con_printf("RCON from %s: %s\n", cl->addr().c_str(), cmdstr.c_str());
	// Setup console's rcon capability
	sv_con_log(cl);
	sv_con_setrcon(1);
	console::exec(cmdstr.c_str());
}

// RCON password
// TODO make a refactored library for this mess
//	the only thing is that i dont know a good way of blending client and server when they are fundamentally split through the client/server model
//	In other words, i'd need to restructure net messages so that they can be defined in other files, rather than being restricted to *_messages.cpp declarations
NET_MESSAGE_TCP(clc_rcon_password) {
	auto& s = cl->stream();
	size_t size = 0;
	s.tcp_recv(size);
	size = min(size, RCON_PASS_SIZE - 1);
	char pwd[RCON_PASS_SIZE];
	net_recv_xor(s, pwd, size, RCON_PASS_KEY, RCON_PASS_KEY_SIZE);
	pwd[size] = '\0';
	// The rcon password is not set, therefore it is not enabled.
	if (!*rcon_password.get_cstr()) {
		static const char rcon_disabled[] = "RCON is disabled on this server\n";
		s.tcp_send(svc_print);
		s.tcp_send(rcon_disabled, sizeof(rcon_disabled));
		cl->tcp_flush(s);
		return;
	}
	if (strcmp(rcon_password.get_cstr(), pwd)) {
		cl->ext().rcon_authed = 0;
		cl->ext().rcon_attempts++;
		// Check if attempts has met or exceeded max attempts
		if (cl->ext().rcon_attempts >= client_ext::rcon_maxattempts)
			sv_kick(cl, "Too many incorrect RCON attempts");
	}
	else {
		if (!cl->ext().rcon_authed) {
			sv_printf(cl, "RCON authed\n");
			con_printf("RCON authed %s\n", cl->addr().c_str());;
		}

		cl->ext().rcon_attempts = 0;
		cl->ext().rcon_authed = 1;
	}
}