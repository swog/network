# network
Nonblocking server and client with nonblocking console input.

## shared
As far as shared goes, these are only headers and source files included in both the server and client.

## client
Client TCP connections are handled by first defining a 'client' type variable. Make sure that it gets destructed eventually by delete or going out of scope.
