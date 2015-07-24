# TCP Context

The Virtual Community Engine (VCE) manages each TCP connection through the use of 2 data structures.

The TCP context data structure (tcpcontext_t) is used to manage multiple TCP connections.

The TCP connection data structure (conn_t) represents a single data socket.


## TCP Context Types

Settings specific to the server (including addresses for binding sockets), settings specific to each client (including connection addresses for each client) as well as settings that relate to both the client and the server are all grouped together as what we will refer to as a TCP context.

Since there are a number of differences in the implementation of TCP/IP networking protocols for clients and servers, VCE has provided separate TCP contexts for them both.

A TCP context can be selected using the first parameter for the vce_tcpcontext_create function. Passing the function a value of 1 selects the server context. Passing the function a value of 0 selects the client context.


## Server Contexts

The server context data structure is used to manage a TCP server.

A single server context provides network services to multiple clients through a single socket which has been bound and is listening for connections (initialized using the bind-listen command).
Server contexts can be used by following the steps listed below.


1. Define a protocol processing routine as needed.
2. Call the vce_tcpcontext_create function using a value of 1 for its first parameter.
3. Using the vce_tcpcontext_set_conn_parser function, register a parser to split data received from the network into individual records and a routine to parse each record (this routine was defined in step 1).
4. Make repeated calls to the vce_heartbeat function.
5. Call the vce_tcpcontext_cleanup function once all processing has finished. In the case of a server, normal operation consists of utilizing an endless loop to continue processing indefinitely.


## Client Contexts

The client context data structure is used to manage TCP clients. In most cases, client contexts can be created by following the steps below.

1. Define a protocol processing routine as needed.
2. Call the vce_tcpcontext_create function using a value of 0 for its first parameter.
3. Using the vce_tcpcontext_set_conn_parser function, register a parser to split data received from the network into individual records and a routine to parse each record (this routine was defined in step 1).
4. Connect to the server using the vce_tcpcontext_connect function.
5. Make repeated calls to the vce_heartbeat function.
6. Call the vce_tcpcontext_cleanup function once all processing has finished.

With the exception of the parameter passed to the vce_tcpcontext_create function, both clients and servers are implemented in a similar fashion.