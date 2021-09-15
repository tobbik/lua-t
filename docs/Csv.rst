lua-t Csv - A Csv reader/writer
+++++++++++++++++++++++++++++++


Overview
========

``t.Csv`` provides the ability to read through a Csv/Tsv file and parse it's
contents.  While the line iterator is provided by the use of Lua
``io.lines()`` the actual value parser is a finite state machine written in
C it should provide good performance even on large and very large files.


Usage
=====

Create a ``Csv`` instance and loop over the ``csv:rows()`` iterator.

.. code:: lua

  Csv = require"t.Csv"
  csv = Csv( )  -- using default settings

  for row in csv:rows( io:lines("myCsvFile.csv") ) do
    -- row is { fld1, fld2, etc }
  end


API
===

Static Class Members
--------------------

``function tokenIterator = Csv.split( string text, string delimiter[, boolean keepDelimiter] )``
  Returns an iterator that returns tokens from ``string text`` separated by
  ``string delimiter`` until the text is exhausted.  ``function
  tokenIterator()`` returns 2 values ``string token, int lineCount``.  This
  is useful to split incoming text sources by newlines before passing them
  to the Csv parser itself.

.. code:: lua

    Csv = require"t.Csv"
    input = "foo,bar,and,so,on"
    for word,i in Csv.split(input, ",") do
      print( i .. "   " .. word )
    end
    -- will print
    --    1   foo
    --    2   bar
    --    3   and
    --    4   so
    --    5   on

  The optional parameter ``boolean keepDelimiter`` defaults to false.  When
  set to ``true``, the tokens returned will be trailed by the delimiter:

.. code:: lua

    Csv = require"t.Csv"
    input = "foo||bar||and||so||on"
    for word,i in Csv.split(input, "||", true) do
      print( i .. "   " .. word )
    end
    -- will print
    --    1   foo||
    --    2   bar||
    --    3   and||
    --    4   so||
    --    5   on     -- last field has no delimiter in input


Class Metamembers
-----------------

``Csv csv = Csv( [string|table delimiter, boolean|table headers, string quotchar, string esacapechar, boolean doublequote, int skip] )   [__call]``
  Instantiate a new ``Csv`` object. All parameters are optional and have
  respective default values.  For detailed descriptions of the parameters
  and their default values refer to the instance members of the same name.
  For more descriptive source code, it is also possible to pass a table that
  contains all relevant fields named like in the instance members like so:

  .. code:: lua

    Csv = require"t.Csv"
    tsv = Csv( {
      delimiter   = "\t",
      quotechar   = "'",
      escapechar  = "\\",
      doublequote = false,
      headers     = { "some", "pre-defined", "headers", "for", "the", "columns" },
      skip        = 0
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

``int skip  = csv.skip``
  Each call to ``csv:rows( function iter )`` will skip ``int skip``
  iterations before starting to parse the payload from the iterator.

``boolean|table headers  = csv.headers``
  If set to ``boolean true`` the parser will read the first line as column
  headers and ``csv.headers`` will be replaced by a table that represents
  the headers in the order of the columns.  If set to ``boolean false`` the
  first line will be interpreted as a line of data values.  If the
  constructor gets passed a ``table headers`` it will not read the first
  line as headers and will use the passed table to define the collumns
  within the rows.  The ``csv.headers`` value effects the ``table rowData``
  which is returned from the row iterator function provided by
  ``csv:rows()``.  If there are no headers it will be an only numerically
  indexed table which holds a value for each column.  If the parser has a
  ``headers`` definition the table will **additionally** contain key/value
  pairs for the indexd data.  The following example illustrates the
  behaviour:

  .. code:: lua

    src=[[
    first,second,third
    a,b,c
    1,2,3]]
    csv=Csv({headers=true})
    for rowData, rowCount in tsv:rows( Csv.split(src) ) do
      ... rowData looks like: {"a","b","c", first="a", second="b", third="c"}
      ... rowData looks like: {"1","2","3", first="1", second="2", third="3"}
    end

``function rowIterator  = csv:rows( function sourceIterator )``
  Rows is an iterator that returns a table of fields for each semantic row
  of the CSV file.  It honours properly encapsulated and escaped line breaks
  in the file itself.  The ``csv:rows()`` iterator returns a
  ``table rowData`` and an ``int rowCount`` for each iteration. For standard
  files it is easiest to use the ``io.lines()`` iterator provided by Lua
  itself:

  .. code:: lua

    Csv = require"t.Csv"
    tsv = Csv( '\t' )
    for rowData, rowCount in tsv:rows( io.lines("data.tsv") ) do
      ... rowData contains all fields of a tsv row
    end

  For convienience to parse text-only sources that are not available as
  files, such as sources received over the network or from a database, the
  ``Csv`` module provides a static ``Csv.split(textData)`` function that can
  be used to create an iterator for string only variables:

  .. code:: lua

    Csv = require"t.Csv"
    csv = Csv( )
    for rowData, rowCount in csv:rows( Csv.split( textCsvData, "\n" ) ) do
      ... rowData contains all fields of a csv row
    end


Instance Metamembers
--------------------

``string s = tostring( Csv csv )  [__toString]``
  Returns a string representing ``Csv csv`` instance.  The string
  contains type, delimiter, quotchar, escapechar, doublequote and memory
  address information, for example: **``T.Csv[<TAB>:":\\:true]``**.
