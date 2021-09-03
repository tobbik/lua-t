lua-t Csv - A Csv reader/writer
+++++++++++++++++++++++++++++++


Overview
========

``t.Csv`` provides the ability to read through a Csv/Tsv file and parse it's
contents.  While the line iterator is provided by the use of Lua
``io.lines()`` the actual value parser is a finite state machine written in
C that should provide very good performance even on large files.


Usage
=====

Create a ``Csv`` instance and the loop over the ``csv:rows()`` iterator.

.. code:: lua

  Csv = require"t.Csv"
  csv = Csv( )

  for row in csv:rows( io:lines("myCsvFile.csv") ) do
    -- row is { fld1, fld2, etc }
  end


API
===

Class Members
-------------

``function tokenIterator = Csv.split( string text, string delimiter)``
  Returns an iterator that returns tokens from ``string text`` separated by
  ``string delimiter`` until the text is exhausted.  ``function
  tokenIterator()`` returns 2 values ``string token, int count``.


Class Metamembers
-----------------

``Csv csv = Csv( [string|table delimiter, string quotchar, string esacapechar, boolean doublequote] )   [__call]``
  Instantiate a new ``Csv`` object. ``delimiter``, ``quotchar``,
  ``escapechar`` and ``doublequote`` are optional parameters.  For detailed
  descriptions of the parameters and their default values refer to the
  instance members of the same name.  For more descriptive source code, it
  is also possible to pass a table that contains all relevant fields named
  like in the instance members like so:

.. code:: lua

  Csv = require"t.Csv"
  tsv = Csv( {
    delimiter   = "\t",
    quotechar   = "'",
    escapechar  = "\\",
    doublequote = false
  } )


Instance Members
----------------

``string delimiter  = csv.delimiter``
  Even though it is a string, only the first character is utilized.  The
  delimiter is used to separate fields.  If the character appears within
  fields it must be either a field that is surrounded by ``quotchar`` or it
  must be prefixed with the ``escapechar``.  The default value is ``,``.

``string quotchar  = csv.quotchar``
  Even though it is a string, only the first character is utilized.  The
  quotchar is used to encapsulate string fields.  If the character appears
  within fields it must be either prefixed with the ``escapechar`` like
  ``\"`` or if ``doublequote`` is true, must be prefixed by another
  ``quotchar`` like ``""``.  The default value is ``"``.

``string escapechar  = csv.escapechar``
  Even though it is a string, only the first character is utilized.  The
  escapechar is used to protect the following character.  If the character
  is meant to be used as value it will be doubled like ``\\``.  It's main
  function is to protect ``delimiter`` and ``quotchar`` if needed.  It also
  changes the meaning of standard control characters such as ``\n`` or
  ``\t``.  The default value is ``\``.

``boolean doublequote  = csv.doublequote``
  If set, ``quotchar`` appearing within a fiels are protected by a
  proceeding ``quotchar``, otherwise the ``escapechar`` is used.

``luaFile/string source  = csv.source``
  This is the file that is currently processed.

``string line  = csv.line``
  The current line the parser works on.

``int state  = csv.state``
  The current state of the parser.  This is mainly an internal value
  accessible for convienience.  Possible values are available in
  src/t_csv_l.h.

``function rowIterator  = csv:rows( function sourceIterator )``
  Rows is an iterator that returns a table of fields for each logical row of
  the CSV file.  It honours properly encapsulated and escaped line breaks in
  the file itself.  the ``csv:rows()``  iterator returns a ``table rowData``
  and an ``int rowCount`` for each iteration.  ``function sourceIterator``
  is mandatory and must be of type iterator that returns a new line of text
  each time it gets called.  For standard files this iseasiest to be used
  with the ``io.lines()`` iterator provided by Lua itself:

.. code:: lua

  Csv = require"t.Csv"
  tsv = Csv( '\t' )
  for rowTable, rowCount in tsv:rows( io.lines("data.tsv") ) do
    ... rowTable contains all fields of a tsv row
  end

For convienience to parse text-only sources that may have been received over
the network or from a database, the ``Csv`` module provides a static
``split()`` function that can be used to create an iterator for string only
variables:

.. code:: lua

  Csv = require"t.Csv"
  csv = Csv( )
  for rowTable, rowCount in csv:rows( Csv.split( textCsvData ) ) do
    ... rowTable contains all fields of a csv row
  end


Instance Metamembers
--------------------

``string s = tostring( Csv csv )  [__toString]``
  Returns a string representing ``Csv csv`` instance.  The string
  contains type, delimiter, quotchar, escapechar, doublequote and memory
  address information, for example: *`T.Csv[<TAB>:":\\:true]:
  0x5650ce588428`*.


