#include "stdafx.h"
#include "server.h"
#include "console.h"
#include "net.h"

static void clc_nop_f(server& sv, size_t cmd, std::shared_ptr<client> cl) {
}

// exit message from client
// no custom message
static void clc_exit_f(server& sv, size_t cmd, std::shared_ptr<client> cl) {
	con_printf("Client disconnected: %s\n", cl->addr().c_str());
	sv.remove(cl);
}

std::vector<clc_msgfn>& clc_messages() {
	static std::vector<clc_msgfn> msgs = {
		clc_nop_f, clc_exit_f, clc_rcon_f, clc_rcon_password_f
	};
	return msgs;
}

// Send a NOP to keep alive
void sv_nop(std::shared_ptr<client> cl) {
	auto s = cl->stream();
	s << svc_nop;
	s.flush();
}

void sv_kick(std::shared_ptr<client> cl, const char* reason) {
	if (!reason)
		reason = "Kicked from server";
	auto s = cl->stream();
	s << svc_exit;
	s << reason;
	s.flush();
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

	auto s = cl->stream();
	s << svc_print;
	s << (const char*)text;
	s.flush();
}
