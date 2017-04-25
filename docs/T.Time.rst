lua-t Time - The Duration object
++++++++++++++++++++++++++++++++++


Overview
========

Time is a simple wrapper around struct timeval.  It provides duration
definition in microsecond resolution and can be used as a simple time
duration definition or a duration definition since the Epoch of 01.01.1970.


API
===

Class Members
-------------

``Time.sleep( int x )``
  make the process sleep for `int x` milliseconds.


Class Metamembers
-----------------

``Time tx = Time( [int x] )   [__call]``
  Instantiate new `Time tx` object.  If an integer is passed as paramter the
  value will be interpreted as milliseconds.  If no paramter is passed a
  `Time` value with the time passed since the Unix epoch will be created.

``Time tx = Time( Time t )   [__call]``
  Instantiate new `Time tx` object.  If another `Time` value is passed a
  clone of that value will be created.


Instance Members
----------------

``void Time t:sleep( )``
  Makes process sleep for however long the instance `Time t` period is set.

``int x = Time t:get( )``
  Returns `Time t` instance duration as `int x` in milliseconds.

``void Time t:set( int x )``
  Set the `Time t` instance duration to `int x` milliseconds.

``void Time t:since( )``
  Set the `Time t` instance duration to the difference between the previous
  value and now in reference to epoch.  It interprets the current value on
  time as duration passed since epoch.  The new value of time will be the
  difference of the time since epoch to now and the original value in the
  timer.  This is a convienience shortcut method if `Time t` was a throwaway
  value anyways or one created for time measurement to begin with.  For
  other applications it is simpler to work with out of the box Time
  subtraction:

  .. code:: lua

    start = Time()
    -- do some busy stuff here
    since = Time() - start

``void Time t:now( )``
  Set the `Time t` instance duration to the difference between now and epoch.


Instance Metamembers
--------------------

``boolean x = (Time t1 == Time t2)  [__eq]``
  Compares `Time t1` and `Time t2` duration objects for equality.  If
  `Time t1` has the same duration as `Time t2` it returns `true`, otherwise
  `false`.

``Time t = Time t1 + Time t2  [__add]``
  Adds the duration of `Time t1` to `Time t2`.  Returns a new instance of
  `Time t`.

``Time t = Time t1 - Time t2  [__sub]``
  Substract the duration of `Time t2` from `Time t2`.  Returns a new
  instance of `Time t`.

``string s = tostring( Time t )  [__toString]``
  Returns `string s` representing `Time t` instance.  `string s` contains
  type, length and memory address information such as
  *`t.Time[0:200000]: 0x1193d18`*, meaning it has a duration of 0 seconds
  and 200000 microseconds.


