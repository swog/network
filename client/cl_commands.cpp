#include "stdafx.h"
#include "console.h"
#include "client.h"
#include "net.h"

// clientside exit command
// notify server
CONSOLE_COMMAND(exit, "Close program", 0) {
	auto& cl = get_client();
	auto s = cl.stream();

	s << clc_exit;
	s.flush();

	get_console().set_open(0);
	cl.close();
}

CONSOLE_COMMAND(rcon_password, "Authenticate with server", 0) {
	// Send password
	if (args.size() < 2) {
		con_printf("Usage: rcon_password <password>\n");
		return;
	}
	size_t size = min(args[1].size(), RCON_PASS_SIZE - 1);
	auto s = get_client().stream();
	s << clc_rcon_password;
	s << size;
	net_send_xor(s, args[1].c_str(), size,
		RCON_PASS_KEY, RCON_PASS_KEY_SIZE);
	s.flush();
}

CONSOLE_COMMAND(rcon, "Type rcon for help", 0) {
	if (args.size() < 2) {
		con_printf("Usage: rcon <command>\n");
		con_printf("Send a remote console command to the server.\n");
		con_printf("For the command to be executed, the client must first authenticate with rcon_password.\n");
		return;
	}
	auto s = get_client().stream();
	s << clc_rcon;
	s << args.argstr;
	s.flush();
}

CONSOLE_COMMAND(status, "Display connection information", 0) {
	auto& svi = get_serverinfo();
	con_printf("IP address: %s\n", get_client().addr().c_str());
	con_printf("Server name: %s\n", svi.name.c_str());
	con_printf("Num clients: %llu\n", svi.numclients);
	con_printf("Max clients: %llu\n", svi.maxclients);
	con_printf("MOTD: %s", svi.motd.c_str());
}