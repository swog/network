#include "stdafx.h"
#include "client.h"
#include "console.h"
#include "messages.h"

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
	auto& s = cl.stream();
	// set nonblocking after connect
	cl.nonblocking(1);
	// Initialize nonblocking console
	get_console();

	const auto& messages = svc_messages;

	while (cl.is_open()) {
		console::flush();
		cl.update();
	}

	console::flush();
}