lua-t T.Buffer - The Buffer class
+++++++++++++++++++++++++++++++++


Overview
========

T.Buffer is a simple wrapper around an allocated memory representing a char
array. It provides a definition in length and some methods to read and write
to it.  Buffer can be utilized as mutable strings.  


USAGE
=====

T.Buffer can be used by T.Net sockets read/write capabilities.  It can also
be used to be examined by T.Pack definition or written to be them which
makes it useful because only parts of the buffer get manipulated instead of
the entries string having to be rewritten.


API
===

Class Members
-------------

None.

Class Metamembers
-----------------

``T.Buffer( int len )   [__call]``
  Instantiate a new T.Buffer object with a value of ``len`` bytes.

``T.Buffer( string myString )``
  Instantiate a new t.Buffer object with the content of ``myString``.  The
  length of ``myString`` defines the length of the T.Buffer instance.


Instance Members
----------------

``string s = bufferInstance:toHex( )``
  Returns a hexadecimal representation of the bytes in the buffer as string.

``string s = bufferInstance:toBin( )``
  Returns a binary representation of the bytes in the buffer as string.

``string s = bufferInstance:read( [int offset, int len] )``
  Returns a string from T.Buffer.  If the ``offset`` paramter is given it
  will read starting form the T.Buffer index ``offset`` otherwise starting
  at index 1.  If ``len`` parameter is given it will read for ``len`` bytes
  from T.Buffer, otherwise until the end of ``T.Buffer``.

``void = bufferInstance:write( string content[, int offset, int len] )``
  Write a string to the bufferInstance.  If the ``offset`` parameter is
  given writing within T.Buffer starts at that value, otherwise it starts at
  index 1.  If ``len`` parameter is given it will write the content starting
  at ``length`` to the T.Buffer, otherwise it starts at the beginning of
  ``content``.

``void = bufferInstance:clear( )``
  Overwrites entire T.Buffer content with 0 bytes.


Instance Metamembers
--------------------

``int x = #bufferInstance  [__len]``
  Returns the length of the T.uffer instance in bytes.

``string s = tostring( bufferInstance )  [__toString]``
  Returns a string representing the buffer instance.  The String contains
  type, length and memory address information such as
  "T.Buffer[234]: 0x1193d18".

``boolean b = T.Buffer t1 == T.Buffer t2  [__eq]``
  Checks if the T.Buffer instances ``t1`` and ``t2`` are equal.  Either by
  reference or by value.

``T.Buffer tR = T.Buffer t1 + T.Buffer t2  [__add]``
  Creates a new T.Buffer instances by combining the contents of ``t1`` and
  ``t2`` are equal.  

