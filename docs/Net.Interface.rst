lua-t t.Net.Interface - Network interfaces
++++++++++++++++++++++++++++++++++++++++++


Overview
========

Since lua-t aims squarely at the embedded market and networking is so
important the library makes an effort to provide reasonable complete
networking support.  The recognition of interfaces (aka. Hardware) is
**very** platform dependent and certain capabilites are simply not available
on all platforms.


API
===

General API remarks
-------------------

The API does not return an actual object that monitors the interfaces in
realtime.  Instead it delivers a snapshot of the interface at the time of
calling.


Content of the snapshot when getting information
------------------------------------------------

Depending on the platform, the following structure can be expected when
calling ``Net.Interface.get(<name>)``:

.. code:: lua

  {
    name    :  ens33,
    AF_INET6: {                         // this is of type t.Net.Address
      address:        T.Net.Address{[fe80::7142:62e5:9a72:8c91]:0}: 0x24b3fe8,
      netmask:        T.Net.Address{[ffff:ffff:ffff:ffff::]:0}: 0x24b4098,
    },
    stats:  {
      multicast:              0,
      rx_frame_errors:        0,
      rx_crc_errors:          0,
      tx_carrier_errors:      0,
      rx_length_errors:       0,
      tx_dropped:             0,
      rx_fifo_errors:         0,
      tx_window_errors:       0,
      rx_errors:              0,
      tx_heartbeat_errors:    0
      tx_errors:              0,
      tx_aborted_errors:      0,
      tx_fifo_errors:         0,
      rx_dropped:             1019470,
      rx_missed_errors:       0,
      tx_bytes:               669813146,
      rx_bytes:               3046649982,
      tx_packets:             1027898,
      rx_over_errors:         0,
      rx_packets:             8466734,
      collisions:             0,
    AF_INET: {                          // this is of type t.Net.Address
      netmask:        T.Net.Address{255.255.252.0:0}: 0x24a0198,
      address:        T.Net.Address{172.16.0.120:0}: 0x24a04b8,
      broadcast:      T.Net.Address{172.16.3.255:0}: 0x24a6f18,
    },
    flags:  {
      IFF_RUNNING:     true,
      IFF_DYNAMIC:     false,
      IFF_MASTER:      false,
      IFF_PROMISC:     false,
      IFF_ALLMULTI:    false,
      IFF_POINTOPOINT: false
      IFF_LOOPBACK:    false
      IFF_PORTSEL:     false
      IFF_UP:          true,
      IFF_MULTICAST:   true,
      IFF_NOARP:       false,
      IFF_SLAVE:       false,
      IFF_AUTOMEDIA:   false,
      IFF_NOTRAILERS:  false,
      IFF_DEBUG:       false,
      IFF_BROADCAST:   true,
    }
  }


Class Members
-------------

``table interfaces = Net.Interface.list( )``
  Return a list of **all** interfaces available on the machine.  The list ir
  organized as a Lua table where each interface is a memebr of the table
  named by it's system intrafce name.  The content of each table is what's
  described above.

``Net.Interface ifc = Net.Interface.get( string name )``
  Gets a new instance ``Net.Interface ifc`` object which has the same structure
  as described above.  The ``string name`` **must equal** the systems own
  interface name, therefore portability is limited.

``Net.Interface ifc = Net.Interface.default( )``
  Gets a new instance ``Net.Interface ifc`` object which could be considered
  the systems default interface.  In order to be considered the default the
  following must be true: Flags IFF_UP, IFF_BROADCAST, IFF_MULTICAST and
  IFF_RUNNING must be true.  If no external interface suffices the
  requirement, ``lo(loopback)`` will be attempted to be returned *IF* it
  suffices above flags.  In case there are more then one qualifying
  interface the one with the most transmitted bytes will be returned.

Class Metamembers
-----------------

``Net.Interface`` has no class members.  There is noticably no constructor,
since it shouldn't be bossible to 'construct' an existing system resource
that already exists.

Instance Members
----------------

As shown above, depending on platform the following *should* be specified:

``table addrs == ifc.AF_INET``
  IPv4 specific addresses named ``address, netmask, broadcast and
  peeraddress``.  The addresses are actual instances of ``t.Net.Address``.

``table addrs == ifc.AF_INET6``
  IPv6 specific addresses named ``address, netmask, broadcast and
  peeraddress``.  The addresses are actual instances of ``t.Net.Address``.

``table flags == ifc.flags``
  SIOCGIFFLAGS flags recieved from a system call.  Each flag is represented
  as a boolean value.

``table statistics == ifc.stats``
  General performance stats on the interface regarding transmitted packets,
  transmitted bytes, error count, collision etc.

Instance Metamembers
--------------------

``string s = tostring( Net.Interface i )  [__toString]``
  Returns ``string s`` representing ``Net.Interface`` instance.
  ``string s`` contains name and IP4 address (if available).  I twill look
  like *`T.Net.Interface{br-963e6d75be2a(172.19.0.1)}: 0x5577dfa00f50`*,
  meaning the systems name is *br-963e6d75be2a* and in it's network it's
  currently connected as IP4 address `172.19.0.1`

