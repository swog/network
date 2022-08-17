#include "stdafx.h"
#include "client.h"
#include "console.h"

#define SERVER_TIMEOUT 20

client& get_client() {
	static client cl;
	return cl;
}

console& get_console() {
	static console _con;
	return _con;
}

int main() {
	auto& cl = get_client();
	cl.connect("127.0.0.1", 1215);
	stream s = cl.stream();
	// set nonblocking after connect
	cl.nonblocking(1);
	// Initialize nonblocking console
	get_console();

	size_t cmd;
	int num;
	const auto& messages = svc_messages();
	time_t tim;

	while (cl.is_open()) {
		console::flush();
		cmd = 0;
		num = s >> cmd;
		tim = time(NULL);
		if (tim - cl.ext().last_send >= SERVER_TIMEOUT - 10)
			cl_nop();
		if (!num) {
			if (tim - cl.ext().last_recv >= SERVER_TIMEOUT) {
				con_printf("Server timed out\n");
				cl_exit();
			}
			continue;
		}
		cl.ext().last_recv = tim;
		if (num != sizeof(cmd))
			continue;
		if (cmd >= messages.size()) {
			con_printf("Message out of bounds %llu (%i)\n", cmd, num);
			continue;
		}
		messages[cmd](cl, cmd);
	}

	console::flush();
}