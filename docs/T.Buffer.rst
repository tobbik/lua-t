lua-t Buffer - The Buffer class
+++++++++++++++++++++++++++++++


Overview
========

``Buffer`` is a simple wrapper around an allocated chunk of memory which is
handled as a char array.  It provides a definition in length and some
methods to read and write to it.  Buffer can be utilized as mutable strings.


USAGE
=====

``Buffer`` can be natively used by ``Net`` sockets read/write capabilities.
It can also be used by ``Pack`` instances to ether read from or write to
them.  This makes it particularity useful because only parts of the buffer
get manipulated instead of the entire string having to be rewritten.


API
===

Class Members
-------------

``number size = Buffer.Size``
  Recomended Sytem Buffer size which is the same as the C constant
  ``BUFSIZ`` as defined in ``<stdio.h>``


Class Metamembers
-----------------

``Buffer buf = Buffer( int length )   [__call]``
  Instantiate a new ``Buffer`` object with a value of ``len`` bytes.

``Buffer  buf = Buffer( string myString )``
  Instantiate a new ``Buffer buf`` object with the content of ``myString``.
  The length of ``myString`` defines the length of the ``Buffer buf``
  instance.

``Buffer clone = Buffer( Buffer cpy_buf )``
  Instantiate a new ``Buffer clone`` object with the content of ``cpy_buf``.
  The length of ``cpy_buf`` defines the length of the ``Buffer clone``.  The
  ``Buffer clone`` is NOT a reference but a true copy of ``cpy_buf``.

``Buffer clone = Buffer( Buffer.Segment cpy_seg )``
  Instantiate a new ``Buffer clone`` object with the content of ``cpy_seg``.
  The length of ``cpy_seg`` defines the length of the ``Buffer clone``.  The
  content of ``Buffer clone`` is copied over from ``cpy_seg``.

``Buffer  buf = Buffer( int length, string myString )``
  Instantiate a new ``Buffer clone`` object with an ``int length`` sized
  copy of the the content of ``myString``.  ``int length`` bytes from
  ``myString`` are copied over to ``Buffer clone``.

``Buffer clone = Buffer( int length, Buffer cpy_buf )``
  Instantiate a new ``Buffer clone`` object with an ``int length`` sized
  copy of the the content of ``cpy_buf``.  ``int length`` bytes from
  ``cpy_buf`` are copied over to ``Buffer clone``.

``Buffer clone = Buffer( int length, Buffer.Segment cpy_seg )``
  Instantiate a new ``Buffer clone`` object with an ``int length`` sized
  copy of the the content of ``cpy_seg``.  ``int length`` bytes from
  ``cpy_seg`` are copied over to ``Buffer clone``.


Instance Members
----------------

``string s = buf:toHex( )``
  Returns a hexadecimal representation of the bytes in the buffer as string.

``string s = buf:toBin( )``
  Returns a binary representation of the bytes in the buffer as string.

``string s = buf:read( [int offset, int len] )``
  Returns a string from `Buffer buf`.  If the `int offset` parameter is
  given it will read starting form the `Buffer buf` at index `int offset`
  otherwise starting at index 1.  If `int len` parameter is given it will
  read for `int len` bytes from `Buffer buf`, otherwise until the end of
  `Buffer buf`.

``void buf:write( string content[, int offset, int len] )``
  Write a string to the `Buffer buf` instance.  If an `int offset` parameter
  is given writing within `Buffer buf` starts at that value, otherwise it
  starts at index 1.  If `int len` parameter is given it will write starting
  `int len` bytes to `Buffer buf`, otherwise all of `string content` will be
  written to `Buffer buf`.

``void = buf:clear( )``
  Overwrites entire ``Buffer buf`` content with *0* bytes.

``Buffer.Segment = buf:Segment( [start, length] )``
  Creates a new Buffer Segment ``Buffer.Segment seg`` content with given
  parameters.  More information in ``Buffer.Segment`` dedicated documentation

Instance Metamembers
--------------------

``int x = #buf         [__len]``
  Returns the length of `Buffer buf` instance in bytes.

``string s = tostring( Buffer buf )  [__toString]``
  Returns a string representing `Buffer bu` instance.  The string contains
  type, length and memory address information, for example:
  *`t.Buffer[234]: 0x1193d18`*.

``boolean b = (Buffer b1 == Buffer b2)  [__eq]``
  Checks if the `Buffer b1` and `Buffer b2` are equal.  Either by
  reference or by value.

``Buffer bR = Buffer b1 + Buffer b2  [__add]``
  Creates a new `Buffer bR` instance by concatenating the contents of `b1`
  and `b2`.

