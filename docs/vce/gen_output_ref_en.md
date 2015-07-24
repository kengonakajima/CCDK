# gen.rb Script C Output Reference

This is a detailed description of the functions created by the gen.rb script for use with C and C++.

The word ```PROTONAME``` will be used throughout this guide to as a placeholder for the name you will be using when defining your own protocol functions.
All functions containing an ```sv``` in their name are for use by the server,
and all functions containing a ```cli``` in their name are for use by clients.

## Functions Requiring Definition, Compilations and Linking in the Target Application

- ```int PROTONAME_sv_sender( conn_t _c, char *data, int len )```
  This function is used to transmit a single data record over the network.

  Parameters:
  ~~~
  conn_t _c: This is the connection which will be used for data transmission.
  char *data: This is a pointer to the data for transmission. This data is serialized and returned by the gen.rb script.
  int len: This is the length, in bytes, of the data to transmit.
  ~~~
  Please make sure to return the length of data that was actually sent. If a negative value is returned, the function begins error handling and the current connection is terminated.

  The example below creates transmission records of up to 65535 bytes and sends them using the unparser_bin16 function.
  ~~~
  int echoproto_sv_sender( conn_t _c, char *data, int len )
  {
    return vce_protocol_unparser_bin16( _c,data,len);
  }
  ~~~
  In the example above, we utilize and send the data serialized by the gen.rb script. The data is sent without the use of any customized processing.

- ```int PROTONAME_sv_recv_error_callback( conn_t _c,int e )```
  This function is called when an invalid record has been received.

  Parameters:
  ~~~
  conn_t _c: This is the connection that was used to receive data.
  int e: This is the error code.
  ~~~
  If a negative number is returned, the connection is terminated. Defining this function allows developers to evaluate the error codes returned and decide whether to delay termination of the connection, write related logs or implement procedures for handling errors.
  The example below writes the error code to standard output and terminates the connection.

  ~~~
  int echoproto_sv_recv_error_callback( conn_t _c,int e )
  {
    vce_errout( "cannot receive data err=%d(%s)\n", e, STRERR );
    return -1;
  }
  ~~~

- ```int PROTONAME_toolong_recv_warning( conn_t c,int proto_id,int length)```

  This function is called when the size of the record received exceeds the maximum record length defined by the protocol. Records of extremely long length may be indicators of a possible network attack and may not necessarily be application or VCE bugs.

  Parameters:
  ~~~
  conn_t _c: This is the connection that was used to receive data.<br>
  int proto_id: This is the ID of the protocol.<br>
  int length: This is the length of the record received.</p>
  ~~~
  The current connection can be terminated by specifying a negative number for the return value of the function.

  The example below terminates the connection without performing any custom processing.
  ~~~
  int echoproto_toolong_recv_warning( conn_t c,int proto_id,int length)
  {
    return -1;
  }
  ~~~

## Callback Functions Requiring Implementation in the Application

For the following functions, addresses for functions which were created by the
gen.rb script need to be notified to VCE via calls to VCE functions.

- ```PROTONAME_sv_pcallback```
  This is a dispatching function that parses the contents of a single record and then makes a Remote Procedure Call (RPC).
The example below uses the vce_tcpcontext_set_conn_parser function to set the tcpcontext_t variable.
~~~
vce_tcpcontext_set_conn_parser( tcpctx,
                                vce_protocol_parser_bin16,
                                PROTONAME_sv_pcallback );
~~~


## Supplemental Constants and Functions

- ```PROTONAME_MAX_CONNECTION```
  This is the maximum number of connections allowed by the server.
- ```PROTONAME_MAX_RECV_RECORD_LENGTH```
  This is the definition of the maximum record length allowed by the server. This value is calculated using the function definitions listed in the protocol definition file. This is one of the first values of the file.

- ```unsigned int PROTONAME_sv_get_version(unsigned int *subv)```
  This function acquires the current version of the of the protocol definition file on the server.
- ```unsigned int PROTONAME_cli_get_version(unsigned int *subv)```
  This function acquires the current version of the protocol definition file on the client.
- ```VCEI64 PROTONAME_get_FUNCNAME_send_count(void)```
  FUNCNAME is a placeholder for the name of each individually defined function. This particular function obtains a count of the number of times a defined function was used for data transmission.
- ```VCEI64 PROTONAME_get_FUNCNAME_recv_count(void)```
  FUNCNAME is a placeholder for the name of each individually defined function. This particular function obtains a count of the number of times a defined function was used to receive data.
- ```void PROTONAME_FUNCNAME_send_debugprint(int on_off)```
  This function toggles the display of debugging information for a defined data transmission function.
- ```void PROTONAME_FUNCNAME_recv_debugprint(int on_off)```
  This function toggles the display of debugging information for a defined function used for receiving data.
  Debugging information is only output if the GEN_DEBUG_PRINT macro was enabled during program compilation.
