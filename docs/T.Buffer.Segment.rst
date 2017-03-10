lua-t T.Buffer.Segment - part of a buffer
+++++++++++++++++++++++++++++++++++++++++


Overview
========

T.Buffer.Segment is a simple wrapper around an existing T.Buffer which
definse only a part of the Buffer with a certain start and a ceratin length.
The majority of the API is exactly like T.Buffer.


USAGE
=====

Like T.Buffer a T.Buffer.Segment can be used by T.Net sockets read/write
capabilities.  It can also be used to be examined by T.Pack definition or
written to be them which makes it useful because only parts of the buffer
get manipulated instead of the entries string having to be rewritten.


API
===

Class Members
-------------

None.

Class Metamembers
-----------------

``T.Buffer.Segment( T.Buffer t, [ int ofs, int len ] )   [__call]``
  Instantiate a new T.Buffer.Segment object.  If ``ofs`` parameter is given
  the new Segment will start at the T.Buffers offset.  If ``len`` parameter
  were given the length of the new Segment is limited to length.


Instance Members
----------------

``string s = bufferSegmentInstance:toHex( )``
  Returns a hexadecimal representation of the bytes in the segment as
  string.

``string s = bufferSegmentInstance:toBin( )``
  Returns a binary representation of the bytes in the segment as string.

``string s = bufferSegmentInstance:read( [int offset, int len] )``
  Returns a string from T.Buffer.Segment.  If the ``offset`` paramter is
  given it will read starting form the T.Buffer.Segment index ``offset``
  otherwise starting at index 1.  If ``len`` parameter is given it will
  read for ``len`` bytes from T.Buffer, otherwise until the end of
  ``T.Buffer.Segment``.

``void = bufferSegmentInstance:write( string content[, int offset, int len] )``
  Write a string to T.Buffer.Segment.  If the ``offset`` parameter is
  given writing within T.Buffer.Segment starts at that value, otherwise it
  starts at index 1.  If ``len`` parameter is given it will write the
  content starting at ``length`` to the T.Buffer.Segment, otherwise it
  starts at the beginning of ``content``.

``void = bufferSegmentInstance:clear( )``
  Overwrites entire T.Buffer.Segment content with 0 bytes.

``T.Buffer b, int ofs, int len = bs:getBuffer( )``
  Retrieve information about the Segment.  The referenced T.Buffer, the
  offset and the length of the segment.


Instance Metamembers
--------------------

``int x = #bufferSegmentInstance  [__len]``
  Returns the length of the T.Buffer.Segment instance in bytes.

``string s = tostring( bufferSegmentInstance )  [__toString]``
  Returns a string representing the buffer segment instance.  The string
  contains type, length and memory address information such as
  "T.Buffer.Segment[12:234]: 0x1193d18".

``boolean b = T.Buffer.Segment t1 == T.Buffer.Segment t2  [__eq]``
  Checks if the T.Buffer.Segment instances ``t1`` and ``t2`` are equal.
  Either by reference or by value.

