lua-t t.Net.Interface - Network interfaces
++++++++++++++++++++++++++++++++++++++++++


Overview
========

Since lua-t aims squarely at the embedded market and networking is so
important the library makes an effor to provide reasonable complete
networking support.  The recognition of interfaces aka. Hardware) is
**very** platform dependent and certain capabilites are simply not available
on all platforms.  


API
===

General API remarks
-------------------

The API does not return an actual object that monitors the interfaces in
realtime.  Instead it delivers a snapshot of the interface at the time of
calling.


Content of the snapshot whne calling the constructor
----------------------------------------------------

Depending on the platform, the following structure can be expected when
callung ``Interface(name)``:

.. code:: lua

  {
    name    :  ens33,
    AF_INET6: {
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
    AF_INET:  {
      peeraddress:    T.Net.Address{172.16.3.255:0}: 0x24a7058,
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

Class Metamembers
-----------------


``Net.Interface ifc = Net.Interface( string name )   [__call]``
  Instantiate new ``Net.Interface ifc`` object which has the same structure
  as described above.  The ``string name`` **must equal** the systems own
  interface name, therefore portability is limited.  The ``string name`` can
  be `default` in which case lu-t tries to figure out which devices is an
  outgoing, and properly configured hardware device.


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
  SIOCGIFFLAGS flags from an ``ioctl()`` system call.  Each flag is
  represented as a boolean value.

``table statistics == ifc.stats``
  General performance stats on the interface regarding transmitted packets,
  transmitted bytes, error count, collision etc.

Instance Metamembers
--------------------

None.
