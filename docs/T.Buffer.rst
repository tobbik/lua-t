lua-t T.Buffer - The Buffer class
+++++++++++++++++++++++++++++++++


Overview
========

T.Buffer is a simple wrapper around an allocated memory representing a char
array. It provides a definition in length and some methods to read and write to
it.  It's companion, the T.Pack class allows for convienient writing and reading
of arbitrary values in the chunk of memory.  Buffer can be utilized as mutable
strings.


API
===

Class Members
-------------

t.Buffer.new( int *len* )
  instantiate a new t.Buffer object with a length of *len* characters.

t.Buffer.new( string *myString* )
  instantiate a new t.Buffer object with the content of *myString*.  The length
  of *myString* defines the length of the buffer.


Class Metamembers
-----------------

t.Buffer( int *len* )   [__call]
  instantiate a new t.Buffer object with a value of *len* characters.

t.Buffer( string *myString* )
  instantiate a new t.Buffer object with the content of *myString*.  The length
  of *myString* defines the length of the buffer.


Instance Members
----------------

int *x* = bufferInstance:length( )
  Returns the length of the buffer instance in bytes.

string *s* = bufferInstance:toString( )
  Returns a string representing the buffer instance.  The String contains type,
  length and memory address information such as "T.Buffer[234]: 0x1193d18".

string *s* = bufferInstance:toHex( )
  Returns a hexadecimal representation of the bytes in the buffer as string.

string *s* = bufferInstance:read( [int *offset*, int *len*] )
  Returns a string from the bufferInstance starting at *offset* with a given
  *length*.  Both, the offset and length parameters are optional.  If no length
  is given, read returns all bytes starting from offset, and if no parameters
  are given, the entire content of the buffer is returned as string.

void = bufferInstance:write( string content, int *offset*, int *len*] )
  Write a string to the bufferInstance starting at *offset* with a given
  *length*.  Both, the offset and length parameters are optional.  If no length
  is given, writes all bytes in the string to the buffer. If no offset is given
  starts at buffer index 1 with writing.


Instance Metamembers
--------------------

int *x* = #bufferInstance  [__len]
  Returns the length of the buffer instance in bytes.

string *s* = tostring( bufferInstance )  [__toString]
  Returns a string representing the buffer instance.  The String contains type,
  length and memory address information such as "T.Buffer[234]: 0x1193d18".

