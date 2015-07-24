# gen.rb Protocol Generator Reference Guide

The gen.rb file is an automated script that creates protocol processing functions for use with the Virtual Community Engine (VCE).
This script simplifies the creation of remote procedure calls (RPC) for VCE.

The gen.rb script reads a simply-formatted protocol definition file and then creates C language source files which can be compiled in Win32 and UNIX environments as well as Ruby language source files for use with Ruby. Since all data transferred between systems is structured using network byte order, the source files are compatible with systems differing in processor architecture or language.

## Table of Contents
- Detailed Description of gen.rb Script Commands
  - <a href="#basic">Basic Concepts</a>
  - <a href="#commandline">Command Line Options</a>
  - <a href="#output">Output File Names</a>

- Protocol Definition File
  - <a href="#infile">Input File Formats</a>
  - <a href="#comment">Comments</a>
  - <a href="#directives">Directives</a>

- Output Code Features
  - <a href="#count">Function Counters</a>
  - <a href="#version">File Versioning</a>
  - <a href="#antihack">Handling Invalid Clients</a>

  
## Detailed Description of gen.rb Script Commands

### <a name="basic"></a>Basic Concepts

The gen.rb script is responsible for generating the function stubs required for enabling RPC calls for VCE.
By defining the functions for use via RPC in this stub file,
switch (or dispatching) functions used for actual remote function calls are also defined.

The TCPcontext data structure is used to:

1. acquire data from a TCP connection
2. use a parsing function to divide the acquired data into individual records
3. call a callback function for each record

Protocol processing is achieved through the steps listed above, but please take note that
the created switch functions correspond to a callback function that is called to process an individual data record.
The use of the gen.rb script reduces bugs and simplifies the development of application communication methods by removing the need to code complicated functions for processing socket data byte by byte.

Additionally, VCE server’s C language implementation and its corresponding client’s Ruby language implementation promote transparent data transmission and simplify network communication programming tasks across multiple platforms.


### <h3><a name="commandline"></a>Command Line Options

The contents of the protocol definition file can be overwritten by specifying a number of command line options.
The name of the protocol definition file is a required option.

~~~
　ruby gen.rb [options] [protocol definition file name]
~~~

The following options are available for use.

~~~
　--protocolname NAME             
　--serverconnection conn|circ
~~~
  

These options are the same as the options used in the protocol definition file and will overwrite the contents of the definition file if used.

~~~
　--noc Disables the output of C language stub files.
　--nocpp Disables the output of C++ language stub files.
　--noruby  Disable the output of Ruby language stub files.</pre>
~~~
  
The options below are flags that change the operation of the gen.rb script.


~~~
  --nocli Disables the output of client files.
  --nosv Disables the output of server files.    
　--identifier STRING</pre>
~~~
  
This option changes the identifier used to interpret functions listed in the protocol definition file. This changes the default value to the string used as a parameter for this option. The default value is “=”.

~~~
　--func_counter_type TYPE</pre>
~~~
  
This option changes the type used by call counter for callback function.

~~~
　--use_const
~~~
  
Add the const type to a number of functions.

### <a name="output"></a>Output File Names

When gen.rb finishes successfully, the following 8 files will be created.

~~~
protocolname_sv.c      a C language source file for server use
protocolname_cli.c     a C language source file for client use
protocolname_sv.cpp    a C++ language source file for server use
protocolname_cli.cpp   a C++ language source file for client use
protocolname_sv.h      a C/C++ header file
protocolname_cli.h     a C/C++ header file
protocolname_sv.rb     a Ruby source file for server use
protocolname_cli.rb    a Ruby source file for client use
~~~


- Files Used When Developing a Server Using C
Compile and link the  ```protocolname_sv.c``` file and include the ```protocolname_sv.h``` file in all source files that will be defining functions for receiving data.

Additionally, the ```protocolname_sv_sender```, ```protocolname_sv_recv_error_callback``` and ```protocolname_toolong_recv_warning``` functions must also be defined (```protocolname``` is a placeholder for the name of a function for receiving data).

Finally, specify a callback function ```protocolname_sv_pcallback``` for any TCPcontext that uses the defined protocol.

Please see our [gen.rb Script C Output Reference](genref_en.md)</a> for more information.

- Files Used When Developing a Client Using C
Compile and link the  ```protocolname_cli.c``` file and include the ```protocolname_cli.h``` file in all source files that will be defining functions for data transmission.

Additionally, the ```protocolname_cli_sender``` and ```protocolname_cli_recv_error_callback``` functions must also be defined (```protocolname``` is a placeholder for the name of a function for data transmission).

Finally, specify a callback function ```protocolname_cli_pcallback``` for any TCPcontext that uses the defined protocol.

Please see our [gen.rb Script C Output Reference](genref_en.md)</a> for more information.

- Files Used When Developing Using C++
Development steps are the same as those for C. However, C++ specific files using the cpp extension can also be used.

- Developing a Server Using Ruby
Use the ```require`` directive to include a ```protocolname_sv.rb``` file.
Provide implementation details for the ```protocol name_sv_sender(_co, 	data )``` , ```protocol name_sv_recv_error_callback(_co, ecode )```, ```protocol name_toolong_recv_warning(_co, protoid, length) methods in the ```protocolname_sv.rb``` source file. The ```$protocolname_sv_proc``` variable should also be defined for use as the second parameter for the ```set_[conn|circ]_parser``` method of a TCPcontext instance. The second parameter for this method is a Proc object.


This Proc instance is called for use in processing records as they are received.
Proc is an object defined in the ```protocolname_sv.rb``` file.

- Developing Clients using Ruby
Use the ```require`` directive to include a ```protocol name_cli.rb``` file.
Provide implementation details for the ```protocol name_cli_sender(_co, data )``` and ```protocol name_cli_recv_error_callback(_co, ecode )``` methods.
　
The ```$protocolname_cli_proc```{1} variable should also be defined for use as the second parameter for the ```set_conn_parser``` method of a TCPcontext instance. The second parameter for this method is a Proc object.
	
This Proc instance is called for use in processing records as they are received.
Proc is an object defined in the ```protocolname_cli.rb``` file.

## The Protocol Definition File

### <a name="infile"></a>Input File Format

The format of the input file can be an HTML, ASCII text, or any other similar type of plain text file.
Lines that begin with an '=' are interpreted as command lines.

Defining protocols in this manner enables developers to define protocols and then use the same file as a protocol definition file when using the gen.rb script.


The gen.rb script parses an input file line by line. If it finds an '=' at the beginning of line, then it retrieves a command and its arguments following the '=' and executes the command.

Any number of spaces after an '=' are perceived as a single space.
A command can span a number of lines by adding a back slash at the end of the line.
The '=' and the command following this symbol must be written on a single line. However, parameters for the command can be broken up into multiple lines as follows:

```=s2c 100 login ( int id, \
    char password[100] ) ```

 
### <a name="comment"></a>Comments

All character written after a '#' character are ignored for any input files used with the gen.rb script.

### <a name="directives"></a>Directives

A line that begins with '=' is called a directive line. A directive line begins with '=' followed by directive strings. The following is a list of available directives.

- ``` protocolname NAME```

  This specifies a protocol name.
Because a protocol name is used in various places such as function or file names, it is recommended to use strings that are clearly recognized as identifiers in the C language.
It is also a good practice to add "proto” as a suffix for protocol functions for identification purposes.
If the protocol name is default then the function name would be defaultproto. Example: ```=protocolname ssproto``` 


- ```serverconnection CONNECTION_TYPE```

  This command specifies the type of connection used by the server. The value for conn is always set in CCDK. ```=serverconnection conn```

- ```version NUMBER```

  This command sets the version of the current protocol. NUMBER can be any whole number.
The specified version number is the return value of the ```int someproto_get_version( int *subv)``` function. See <a href="#version">File Versioning</a> for more information.

- ```idmax NUMBER```

  Specifies the maximum ID number for protocol functions.
  The size of the internal data structure for function IDs will be 1 byte if the maximum size is equal to or less than 255, 2 bytes if it is in an inclusive range from 256 to 65535 and 4 bytes if the number is greater than or equal to 65536.

It is best to keep this number small in order to reduce network traffic.
The default value is 255. An error will be raised if function IDs greater than 255 are specified in s2c or c2s directives.


- ```define IDENTIFIER EXPRESSION```
  This defines a macro. At present, it is only supported in C.
Macros are defined as ``` =define TEST 1 ```. This is an argument for functions and mainly used to define array subscripts.
Complicated macros like ``` =define TEST_A(x) (x+1) ``` cannot be defined using this method.
Defining macros using this method will create #define directive similar to ```#define SSPROTO_IDENTIFIER EXPRESSION``` in both the ssproto_sv_h and ssproto_cli.h files.

By defining error codes and size constants that are frequently used in protocol definitions, clients and servers can share a constant definition file increasing development efficiency.


- ```include FILENAME```
  This reads in and inserts the specified file at the location that this directive is used.
This can be used for reading common macro definitions.
Example: ```=include "common.txt" ```

- ```static IDENTIFIER EXPRESSION```

  Complicated macros that cannot be using the define directive cannot be defined using this method either.
The main purpose of this functionality is for the definition of macros for use in files included using the include directive.
These will be defined in xxproto_sv.h and xxproto_cli.h as ```#define xx_IDENTIFIER EXPRESSION ``` which is similar to the define directive.
The difference between this and the define directive is xx will be replaced with the name of the file used for the include directive.

  If the following is defined in a file named common.txt
~~~
=static ID_LEN 32
~~~
and the following is defined in a file named testproto.txt
~~~
=protocolname testproto
=include "common.txt"
=define MAX_LEN 64          
~~~
then the following would be output to the testproto_sv.h and testproto_cli.h files:
~~~
#define COMMON_ID_LEN 32
#define TESTPROTO_MAX_LEN 64
~~~
 

- ```ifdef IDENTIFIER FUNCNAME```

  This allows for the selection of protocols based on other defined macros.

  ``` =ifdef _DEBUG ping ```

  This will enable the ping transmission and reception functions on both client and server if _DEBUG has been defined for the preprocessor.


- ```enum ENUMTYPE IDENTIFIER [EXPRESSION]```

  Similar to the define directive. This defines enum for C.
~~~
=enum HOGE INVALID_ARGUMENT -999
=enum HOGE NOT_SUPPORTED
~~~

  If the enums above have been defined and the name of the protocol is GAMEPROTO,
~~~
typedef enum {
    GAMEPROTO_INVALID_ARGUMENT=-999,
    GAMEPROTO_NOT_SUPPORTED,
} GAMEPROTO_HOGE;
~~~

  then an enum with the prefix of GAMEPROTO_HOGE will be defined.
More than one ENUMTYPE can be specified, in which case multiple enums will be defined.
If a an expression such as (-?[0-9]+) is specified for EXPRESSION, it will be evaluated as a value for an enum. 
If no value is specified, an enum with a default value is created.


- ```struct IDENTIFIER{ ARGDECL } ```

  Similar to the define directive. This defines structures for C.
~~~
=struct hoge { int x, int y }
~~~
If a structure has been defined in a similar fashion to the example above
~~~
typedef struct _hoge {
    int x,
    int y,
} hoge;
~~~
then a structure similar to the text above will be defined.
<b>ARGDECL</b> is used to define structure members.
Parameters with types similar to the example below or data structures can be used.</p>
~~~
=struct hoge { int a, int b[20], char c[21], string d[22], stringarray e[23][24] }
~~~
Using the struct directive as illustrated above creates definitions in the gen.rb script as listed below.
~~~
typedef struct _hoge {
    int a;
    int b[20];
    int b_len;
    char c[21];
    int c_len;
    char d[22];
    char e[23][24];
    int e_len;
    char *e_p;
    int e_i;
} hoge;
~~~

- ```c2s ID FUNCNAME ( ARGDECL )``` or ```s2c ID FUNCNAME ( ARGDECL )```

  <B>c2s</B> and <B>s2c</B> are function definitions generated by the gen.rb script. c2s and s2c can be written in a single file with a total function count of idmax function definitions.
c2s is an abbreviation for "client to server," which is used to define transmission functions on the client side and receiving functions on the server side.
In other words, if you write a c2s directive, then a transmission function is added to ```protocol name_cli.c``` and a receiving function call (not a definition) is added to ```protocol name_sv.c```.

The reason that only function calls are written to the ```protocolname_sv.c``` file is because, the implementation of each function has to be created in the application. Thus, the ```protocolname_sv.c``` file only makes function calls.
A message <B>ID</B> with a value less than idmax is specified.
A message ID is used by then client and server to identify a function, and it is possible to handle up to idmax functions. This is a count of both s2c and c2c functions.
This ID can be added manually to maintain compatibility with the previous protocol versions when a new function is added to the protocol.
Skipping ID numbers is recommended if you plan to add additional functions.

  <B>FUNCNAME</B> specifies a function name.
Strings that follow the conventions of the C language must be used for function names.

  <B>ARGDECL</B> specifies parameters for each individual function.
The following parameter types are available for use.
~~~
int i: an integer
uint i: an unsigned integer
short i: a short integer
ushort i: an unsigned short integer
char i: an integer represented as a character
uchar i	: unsigned integer represented as a character
int64 i	: a 64 bit integer
float i	: a floating point number (successful transmission of this type is dependent on the capabilities of your running environment)
int a[100]: an integer array
short a[100]: a short integer array
char a[100]: a character array
float a[100]: a floating point number array (successful transmission of this type is dependent on the capabilities of your running environment)
string a[100]: a string array
stringarray a[100][100]: a string array [maximum number of strings][maximum number of characters for each string]
~~~

  A size must be specified when you declare an array.
This is done to prevent buffer overflows in the work region allocated by the gen.rb script.
Note that the work region is created on the stack, so setting a large value could result in a stack overflow error.
However, a number that is in an order of several thousand bytes should be handled with no problems.

  Since arrays are defined with a maximum size, the full amount of data may not actually be transmitted.
For example, if the first 10 bytes of the character array char a[100] is used, only 10 bytes will be transmitted.
Also, if an array is specified as an argument, an int argument indicating the number of elements in the array is automatically added.
The value is stored in a variable named variablename_len.

  The following is an example of configurations and functions to be generated. someproto is the name of the protocol.
~~~
=s2c 100 login( int id, char passwd[20] )
~~~
The configuration example above outputs the following C code
~~~
int someproto_login_send( conn_t _c, int id, char *passwd, int passwd_len )
~~~
and the following Ruby method
~~~
someproto_login_send( Conn c, Fixnum id, String passwd )
~~~
The following example
~~~
=c2s 101 sendbig( int bigdata[1000] )
~~~
outputs the following C code
~~~
int someproto_sendbig_send( conn_t _c, int *bigdata, int bigdata_len )
~~~
and the following Ruby method
~~~
someproto_sendbig_send( Conn c, Array ia )
~~~
The maximum size of an array is not its byte count but the count of the elements in the array.

- ```=sh Code```  or ```=ch Code```

  <B>sh</B> adds code to the server header file.
<B>ch</B> adds code to the client header file.
For example, adding the following to the (testproto.txt) protocol file,
~~~
=sh #include "server.h"
=sh extern Server * sv;
=sh extern Server2 sv2;
=ch #include "client.h"
=ch extern Client * cli;
=ch extern Client2 cli2;
~~~  in a definition file named "testproto.txt",
is converted as listed below for the testproto_sv.h file　
~~~
#ifndef _TESTPROTO_SV_H_
#define _TESTPROTO_SV_H_
#include "vce.h"
#include "server.h"
extern Server * sv;
extern Server2 sv2;
#undef TESTPROTO_MAX_CONNECTION
#define TESTPROTO_MAX_CONNECTION 1024
....
~~~
and as listed below for the testproto_cli.h file
~~~
#ifndef _TESTPROTO_CLI_H_
#define _TESTPROTO_CLI_H_
#include "vce.h"
#include "client.h"
extern Client * cli;
extern Client2 cli2;
#ifdef __cplusplus
extern "C" {
....
~~~

        
## Output Code Features
		### <a name="count"></a>Function Counter


The gen.rb script defines a function that counts how many times each defined sending and receiving command is used.
When the "login” function is used to send login information from the client to the server, the following C code
~~~
VCEI64 someproto_get_login_send_count(void)
~~~
and the following Ruby method
~~~
someproto_get_login_send_count()
~~~
and on the server the following C code
~~~
VCEI64 someproto_get_login_recv_count(void)
~~~
and the following Ruby method
~~~
someproto_get_login_recv_count()
~~~
By default, the type of return value for the counter is VCEI64, but it can be changed by using the```--func_counter_type``` command line option or the ```=func_counter_type``` directive.

### <a name="version"></a>File Versioning

File versioning is indispensable since the definition for the protocol is constantly changing, 
We suggesting managing the version numbers of protocols for the gen.rb script using two values. The first value is a version number that is specified by programmers, and the other value is a sub version number that is generated by hashing the contents of the configuration file. This can be used as a method to fingerprint the content of a file.

The version number is specified using the ```=version``` directive.
This is referred to as the major version number.

Both sub and major version numbers can be retrieved using the following functions.

On the server, you can use the following function for C
~~~
unsigned int someproto_sv_get_version(unsigned int *subv )
~~~
 and the following function for C for the client
~~~
unsigned int someproto_cli_get_version(unsigned int *subv )
~~~
On the server you can use the following Ruby method
~~~
someproto_sv_get_version()
~~~
and the following Ruby method on the client
~~~
someproto_cli_get_version()
~~~

These functions return the major version that is specified in a definition file and the minor version which is an integer value retrievable using the subv pointer. A sub version is only used when a strict content match is required for the protocol definition file.
This value can normally be ignored by specifying NULL as the parameter for subv.

If ```=version``` has not be defined in a configuration file, this function always returns 0xffffffff as the major version number.

The Ruby method always returns an array containing two values. The element located at position 0 is the major version and the element at position 1 is the sub version.


Example of Use

~~~
// version: major version number, hashver : sub version        
unsigned int hashver;
unsigned int version = someproto_sv_get_version(&hashver);  
~~~
        

- <a name="antihack"></a>Handling Invalid Clients

There are instances when a user may attempt to cheat or attack the service by using invalid packets.
This is especially true if the program has been modified and the protocol has been analyzed so that packets requiring expensive resources for processing are repeatedly send to the server with the intent of crashing the it (a DoS attack).

In order to reduce the load on the server’s transmission buffer, extremely long protocols should be filtered out as soon as possible.
```protcolname_MAX_RECV_RECORD_LENGTH``` is defined for the server header file by the gen.rb script by selecting the longest record in the script.

Using this constant with the ```vce_tcpcontext_protocol_set_maxlength``` function rejects extremely long protocol calls.

The ```protocolname_toolong_recv_warning``` function is also called when an unexpectedly long data packet is received.
