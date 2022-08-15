# network
Nonblocking server and client with nonblocking console input.

Note that there are many additions made to apply the networking library. For example, linkage requires that the client/server object be exposed globally. To do this, I included the functions: 'get_client', 'get_server'. Additionally, 'get_console' is defined by the console system.

## shared
As far as shared goes, these are only headers and source files included in both the server and client.
Both the client and server classes were intended to be easily mutable.

## stream

## client
Client TCP connections are handled by first defining a 'client' type variable. Make sure that it gets destructed eventually by delete or going out of scope.
See: client/cl_main.cpp

To connect a client, I recommend enabling blocking (enabled upon initialization), then using the 'connect' method with a string ipv4 and 16 bit host port/service.
After connection, the 'is_open' method will return true as long as the internal socket is not invalidated by closure.

## server
First instantiate a 'server' object with a port number and connection cap, use 'SOMAXCONN' for the maximum number of connections.

```cpp
// Blocking client acceptance:

int main() {
	server sv(1818, 3);
	
	client cl;
	sv >> cl;
	stream s = cl.stream();
	
	std::string str;
	s >> str;
	printf("%s\n", str.c_str());
}
```

```cpp
// Nonblocking client acceptance

static void onconnect(server& sv, std::shared_ptr<client>& cl) {
	printf("Client connected: %s\n", cl->addr().c_str());
}

int main() {
	server sv(1818, 3);
	sv.nonblocking(1);
	sv.set_onconnect(onconnect);
	size_t cmd = 0;
	
	static const auto& messages = clc_messages();
	
	while (sv.is_open()) {
		sv.update();
		for (size_t i = 0; i < sv.size(); i++) {
			auto& cl = sv[i];
			cmd = 0;
			cl >> cmd;
			if (cmd >= messages.size())
				continue;
			messages[cmd](sv, cmd, cl);
		}
	}
}
```

## console
