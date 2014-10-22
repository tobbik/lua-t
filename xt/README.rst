========
OVERVIEW
========


What is it
----------

lua-xt is a library which extends lua by some universal functionality
including the following topics:

 - Networking (xt.Socket) --> TCP,UDP etc
 - Buffers (xt.Buffer)    --> a defined buffer of length x withmutable values
 - Packers (xt.Packer)    --> a Packer of type and size
 - Struct (xt.Packer.Struct) --> a Combination of Packers or Structs
 - Array (xt.Packer.Struct)  --> a Sequence of Packers or Structs of same type
 - Encoding/Decoding/Encryption (xt.Encoding) --> CRC,Base64,RC4...
 - Unit Tests (xt.Test) --> comprehensive tests with unified output
 - Asynchronous (xt.Loop) --> event/select based asynchronous workings





Coding Standards
+++++++++++++++

OOP interface
-------------

luax-xt uses a unified OOP type according to Luas own recomendation for
objects. Class.new() or Class() create new objects


C-CLass
-------

Prefixes:

 - xt               --> part of the xt library
 - xt_abc           --> part of the xt_xxx class
 - lxt_abc          --> exposed to the Lua API

 Exposure Level:

 - lxt_abc_Func     --> xt.Abc.Func()  static method on xt.Abc
 - lxt_abc__Func   --> metatable function of xt.Abc such as xt.Abc() (__call method)
 - lxt_abc_func     --> method of xt.Abc instance such as myAbc:func() 
 - lxt_abc__func   --> metatable method of xt.Abc instance (#myAbc length)




Examples:

xt_xxx_new() 





Documentation:
++++++++++++++

xt.Packer
---------

A Packer is a simple Data Format definition containing size and type. The following types are available:

 - xt.Packer.Bit(x,y)  = creates a packer of type Bit with big Endianess and length x and offset y
 - xt.Packer.Int(x)    = creates a packer of type Int with native Endianess and length x
 - xt.Packer.IntL(x)   = creates a packer of type Int with little Endianess and length x
 - xt.Packer.IntB(x)   = creates a packer of type Int with Big Endianess and length x
 - xt.Packer.Float(x)  = creates a packer of type Float with length x
 - xt.Packer.String(x) = creates a packer of type String with length x






