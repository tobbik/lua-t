lua-t t.Net.Address - Network/Socket Adresses
+++++++++++++++++++++++++++++++++++++++++++++


Overview
========

lua-t provides a special Class that can create object instances which
represent network addresses.  On the C-side of the library this userdata is
straight mapped to ``struct sockaddr_storage`` instances which are
reasonably portable.  There are essentially three types of information
available:

 - Family (AF_INET, AF_INET6, AF_UNIX)
 - Ip (127.0.0.1, ::1, /tmp/mySock)
 - Port ( if applicable )


API
===

General API remarks
-------------------

While it is possible to to change the family of an ``Net.Address`` instance
it is only necessary in uncommon cases.  When an IPv4 or IPv6 gets assigned
the family is automatically inferred from the IP address' format.


Class Members
-------------

None.

Class Metamembers
-----------------

``Net.Address adr = Net.Address( [string ip, int port] )   [__call]``
  Instantiate new ``Net.Address adr`` object.  If no ``string ip`` argument
  is given the constructor will create a AF_INET (IPv4) address which has
  ``0.0.0.0(INADDR_ANY)`` as value.  If no ``int port`` is given port will
  be set to 0.  Depending on the format of ``string ip`` the ``Net.Address``
  instance will be either of IPv4 or IPv6 type.

Instance Members
----------------

As shown above, depending on platform the following *should* be specified:

``string family == adr.family``
  The family of the ``Net.Address`` formatted as string according to all the
  types of the ``AF_*`` members.

``string ip == adr.ip``
  The destination of the ``Net.Address`` formatted according to the
  convention of the value ``adr.family``.  If an address in the form of
  AF_INET(IPv4) or AF6_INET(IPv6) is passed the value of ``adr.family`` gets
  set automatically accordingly.

``integer port == adr.port``
  If applicable, the port of the ``Net.Address`` as integer.

``integer scope == adr.scope               [readonly]``
  If applicable, the scope id of the ``Net.Address`` as integer.  This value
  is only available for IPv6(AF_INET6) family addresses.

``integer flow == adr.flow               [readonly]``
  If applicable, the flow information of the ``Net.Address`` as integer.
  This value is only available for IPv6(AF_INET6) family addresses.


Instance Metamembers
--------------------

``string s = tostring( Net.Socket sck )  [__tostring]``
  Returns a string representing the ``Net.Address`` instance.  The string
  contains ``ip:port`` and memory address information such as
  ``*t.Net.Address[[::1]:8000]: 0xdac2e8*``, meaning it is an AF6_INET
  (IPv6) Address with the destination localhost and port 8000.

``boolean x = Net.Address adr1 == Net.Address adr2  [__eq]``
  Compares two ``Net.Address`` instances for equality.  If ``Net.Address
  adr1`` is of the same family with the same ip and, if applicable, the same
  port the addresses are considered equal.


