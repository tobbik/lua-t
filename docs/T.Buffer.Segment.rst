lua-t Buffer.Segment - part of a buffer
+++++++++++++++++++++++++++++++++++++++


Overview
========

``Buffer.Segment`` is a simple wrapper around an existing ``Buffer`` which
defines only a part of the ``Buffer`` with a certain start and a certain
length. The majority of the API is identical to the `Buffer` API, in fact
most of the underlying functions are in fact the same.  Most noticeably,
``Buffer.Segment`` instances do not have an ``__add()`` metamethod because
any implementation would leave too much ambiguity.


USAGE
=====

A `Buffer.Segment` instance can be used exactly like an actual `Buffer` with
regards to interoperability with the `Net.Socket` and the `Pack` class.

Schema
======

Showing the layout of a Segment created from a buffer with the following
values: ``seg = buffer:Segment( 6, 7 )``. It results in a Segment that looks
like this: ``seg.start == 6``, ``seg.size == 7`` and the ``seg.last == 12``

idxBuf:  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
values:  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W
idxSeg:                 1  2  3  4  5  6  7


API
===

Class Members
-------------

None.

Class Metamembers
-----------------

``Buffer.Segment seg = buf:Segment( [ int ofs, int len ] )   [__call]``
  Instantiate a new ``Buffer.Segment`` object.  If ``int ofs`` parameter is
  given the new ``Buffer.Segment`` will start at the ``Buffer buf`` offset.
  If ``int len`` is given the length of the new ``Buffer.Segment`` is
  limited to length.


Instance Members
----------------

``Buffer buf  = seg.buffer``
  ``Buffer buf`` which ``Buffer.Segment seg`` refers to.  This value is
  read-only.

``int start = seg.start``
  Starting index of ``Buffer.Segment seg`` relative to ``Buffer
  seg.buffer``.  This value can be changed.

``int start = seg.size``
  Number of bytes in ``Buffer.Segment seg``.  This value can be changed.  It
  has the same value as ``#seg``, but accessed as ``seg.size`` it is mutable.

``int lastIndex = seg.last``
  Ending index of ``Buffer.Segment seg`` relative to ``Buffer seg.buffer``.
  It is defined by ``seg.start + seg.size - 1``.  This value can be changed.

``string s = seg:toHex( )``
  Returns a hexadecimal representation of the bytes in the segment as
  string.

``string s = seg:toBin( )``
  Returns a binary representation of the bytes in the segment as string.

``string s = seg:read( [int offset, int len] )``
  Returns a string from ``Buffer.Segment seg``.  If ``int offset`` is given
  it will read starting from the ``Buffer.Segment seg`` index ``int offset``
  otherwise starting at index 1.  If ``int len`` is given it will read for
  ``int len`` bytes from ``Buffer.Segment seg``, otherwise until the end of
  ``Buffer.Segment seg``.

``void = seg:write( string content[, int offset, int len] )``
  Write a string to ``Buffer.Segment seg``.  If ``int offset`` is given
  writing within ``Buffer.Segment seg`` starts at that value, otherwise it
  starts at index 1.  If `int len` parameter is given it will write
  ``int len`` bytes of ``string content``, otherwise the entire content.

``void = seg:clear( )``
  Overwrites entire ``Buffer.Segment seg`` content with 0 bytes.

``boolean success = seg:shift( int )``
  Shifts the segment within the Buffer. Error if ``seg.start < 1`` or if
  ``seg.start+seg.size > #seg.buffer``

``boolean success = seg:next( )``
  Forwards the ``Buffer.Segment seg`` right adjacent to the current
  position. ``seg`` has the same lenght as seg if there is enough space left
  in ``seg.buffer`` otherwise it has the reminder of of
  ``seg.buffer`` which is defined by ``#seg = #seg.buffer-seg.last``


Instance Metamembers
--------------------

``int x = #seg  [__len]``
  Returns the length of the T.Buffer.Segment instance in bytes.

``string s = tostring( seg )  [__toString]``
  Returns a string representing ``Buffer.Segment seg`` instance.  The string
  contains type, offset, length, and memory address information such as
  *`t.Buffer.Segment[12:234]: 0x1193d18`*.

``boolean b = Buffer.Segment bs1 == Buffer.Segment bs2  [__eq]``
  Checks if the ``Buffer.Segment b1`` and ``Buffer.Segment b2`` are equal.
  Either by reference or by value.

``integer byte = Buffer.Segement seg[ integer n ]    [__index]``
  Returns the value of the byte at position ``n``.

``Buffer.Segment seg[ integer n ] = integer byte    [__newindex]``
  Sets the value of the byte at position ``n``. The value of ``byte`` must
  be between 0 and 255.

``Buffer.Segment segA == Buffer.Segment segB [__eq]``
  Measures equality between segments.  The operation returns true if lenght
  and content of both Segments are identical.  This does not compare
  reference values but actual values.

