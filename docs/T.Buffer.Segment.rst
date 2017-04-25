lua-t Buffer.Segment - part of a buffer
+++++++++++++++++++++++++++++++++++++++


Overview
========

`Buffer.Segment` is a simple wrapper around an existing `Buffer` which
defines only a part of the `Buffer` with a certain start and a certain
length. The majority of the API is identical to the `Buffer` API, in fact
most of the underlying functions are in fact the same.  Most noticeably,
`Buffer.Segment` instances do not have an `__add()` metamethod because any
implementation would leave too much ambiguity.


USAGE
=====

A `Buffer.Segment` instance can be used exactly like an actual `Buffer` with
regards to interoperability with the `Net.Socket` and the `Pack` class.


API
===

Class Members
-------------

None.

Class Metamembers
-----------------

``Buffer.Segment( Buffer t, [ int ofs, int len ] )   [__call]``
  Instantiate a new `Buffer.Segment` object.  If `int ofs` parameter is
  given the new `Buffer.Segment` will start at the `Buffer buf` offset.  If
  `int len` is given the length of the new `Buffer.Segment` is limited to
  length.


Instance Members
----------------

``string s = seg:toHex( )``
  Returns a hexadecimal representation of the bytes in the segment as
  string.

``string s = seg:toBin( )``
  Returns a binary representation of the bytes in the segment as string.

``string s = seg:read( [int offset, int len] )``
  Returns a string from `Buffer.Segment seg`.  If `int offset`` is given it
  will read starting from the `Buffer.Segment seg` index `int offset`
  otherwise starting at index 1.  If `int len` is given it will read for
  `int len` bytes from `Buffer.Segment seg`, otherwise until the end of
  `Buffer.Segment seg`.

``void = seg:write( string content[, int offset, int len] )``
  Write a string to `Buffer.Segment seg`.  If `int offset` is given
  writing within `Buffer.Segment seg` starts at that value, otherwise it
  starts at index 1.  If `int len` parameter is given it will write
  `int len` bytes of `string content`, otherwise the entire content. 

``void = seg:clear( )``
  Overwrites entire `Buffer.Segment seg` content with 0 bytes.

``Buffer buf, int ofs, int len = seg:getBuffer( )``
  Retrieve information about the Segment.  Returns referenced `Buffer buf`,
  the `int offset` and `int len`.


Instance Metamembers
--------------------

``int x = #seg  [__len]``
  Returns the length of the T.Buffer.Segment instance in bytes.

``string s = tostring( seg )  [__toString]``
  Returns a string representing `Buffer.Segment seg` instance.  The string
  contains type, offset, length, and memory address information such as
  *`t.Buffer.Segment[12:234]: 0x1193d18`*.

``boolean b = Buffer.Segment bs1 == Buffer.Segment bs2  [__eq]``
  Checks if the `Buffer.Segment b1` and `Buffer.Segment b2` are equal.
  Either by reference or by value.

