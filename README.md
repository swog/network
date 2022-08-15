# network
Nonblocking server and client with nonblocking console input.

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

## console
