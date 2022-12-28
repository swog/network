#include "stdafx.h"
#include "server.h"
#include "console.h"
#include "net.h"

NET_MESSAGE_TCP(clc_nop) {
}

// exit message from client
// no custom message
NET_MESSAGE_TCP(clc_exit) {
	con_printf("Client disconnected: %s\n", cl->addr().c_str());
	sv.remove(cl);
}

NET_MESSAGE_TCP(clc_clientinfo) {
	auto& s = cl->stream();
	auto& cli = cl->ext().client_data;
	s.tcp_recv(cli.name);
	cl->tcp_flush(s);
}

std::vector<net_message*> clc_messages = {
	(net_message*)&clc_nop_inst,
	(net_message*)&clc_exit_inst,
	(net_message*)&clc_rcon_inst,
	(net_message*)&clc_rcon_password_inst,
	(net_message*)&clc_clientinfo_inst,
};

// Send a NOP to keep alive
void sv_nop(std::shared_ptr<client> cl) {
	auto& s = cl->stream();
	s.tcp_send(svc_nop);
	cl->tcp_flush(s);
}

void sv_kick(std::shared_ptr<client> cl, const char* reason) {
	if (!reason)
		reason = "Kicked from server";
	auto& s = cl->stream();
	s.tcp_send(svc_exit);
	s.tcp_send(reason, strlen(reason) + 1);
	cl->tcp_flush(s);
	con_printf("Kicked %s: %s\n", cl->addr().c_str(), reason);
	// Remove the client from the client vector
	// This removes the main reference to the client, causing it to be freed once all references have subsided
	get_server().remove(cl);
}

void sv_printf(std::shared_ptr<client> cl, const char* format, ...) {
	static char text[1024];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(text, format, ap);
	va_end(ap);

	auto& s = cl->stream();
	s.tcp_send(svc_print);
	s.tcp_send(text, strlen(text) + 1);
	cl->tcp_flush(s);
}
