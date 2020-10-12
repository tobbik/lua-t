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
    mtu        : 1500,                -- MTU as reported by ioctl()
    hw_address : "ab:cd:ef:12:34:56", -- MAC Address
    index      : 2,                   -- system index of interface
    name       : "enp3s0",            -- device name
    flags      : {                    -- Flags of interface
      IFF_DYNAMIC        : false,
      IFF_UP             : true,
      IFF_AUTOMEDIA      : false,
      IFF_NOARP          : false,
      IFF_POINTOPOINT    : false,
      IFF_LOOPBACK       : false,
      IFF_NOTRAILERS     : false,
      IFF_PORTSEL        : false,
      IFF_MULTICAST      : true,
      IFF_RUNNING        : true,
      IFF_BROADCAST      : true,
      IFF_SLAVE          : false,
      IFF_PROMISC        : false,
      IFF_MASTER         : false,
      IFF_ALLMULTI       : false,
      IFF_DEBUG          : false
    },
    stats: {                           -- reported stats
      tx_aborted_errors  : 0,
      tx_bytes           : 10103771,
      tx_packets         : 112181,
      multicast          : 45,
      tx_window_errors   : 0,
      collisions         : 0,
      tx_carrier_errors  : 0,
      tx_dropped         : 0,
      rx_over_errors     : 0,
      rx_missed_errors   : 0,
      rx_frame_errors    : 0,
      tx_heartbeat_errors: 0,
      tx_fifo_errors     : 0,
      rx_crc_errors      : 0,
      rx_dropped         : 0,
      rx_packets         : 225599,
      rx_errors          : 0,
      tx_errors          : 0,
      rx_bytes           : 296155550,
      rx_fifo_errors     : 0,
      rx_length_errors   : 0
    },
    AF_INET: {                          -- table of AF_INET(IPv4) addresses
      {
        netmask   : T.Net.Address{255.255.255.0:0}: 0x55eeacd1ebc8,
        address   : T.Net.Address{192.168.17.197:0}: 0x55eeacd1eb08,
        broadcast : T.Net.Address{192.168.17.255:0}: 0x55eeacd1ecc8
      }
    AF_INET6:                           -- table of AF_INET6(IPv6) addresses
      {
        netmask   : T.Net.Address{[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:0}: 0x55eeacd1f3b8,
        address   : T.Net.Address{[fd53:43e:dbca::60a]:0}: 0x55eeacd1f2f8
      }, {
        netmask   : T.Net.Address{[ffff:ffff:ffff:ffff::]:0}: 0x55eeacd1f5b8,
        address   : T.Net.Address{[fd53:43e:dbca:0:de1f:bea:1f79:7d49]:0}: 0x55eeacd1f4f8
      }, {
        netmask   : T.Net.Address{[ffff:ffff:ffff:ffff::]:0}: 0x55eeacd1f7e8,
        address   : T.Net.Address{[fe80::641c:193a:54ff:2ea6]:0}: 0x55eeacd1f728
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
  A table of IPv4 specific addresses. Each table contains the fields for
  ``address``, ``netmask``, ``broadcast`` or ``peer``.  The latter are
  mutually exlusive.  The addresses are actual instances of
  ``t.Net.Address`` and the family ``AF_INET``.

``table addrs == ifc.AF_INET6``
  A table of IPv6 specific addresses. Each table contains the fields for
  ``address``, ``netmask`` The addresses are actual instances of
  ``t.Net.Address`` and the family ``AF_INET6``.

``t.Net.Address addr == ifc.address``
  A convienience accessor to ``ifc.AF_INET[1].address`` or as a fallback to
 ``ifc.AF_INET6[1].address``.

``table flags == ifc.flags``
  SIOCGIFFLAGS flags recieved from a system call.  Each flag is represented
  as a boolean value.

``table statistics == ifc.stats``
  General performance stats on the interface regarding transmitted packets,
  transmitted bytes, error count, collision etc.

``int mtu == ifc.mtu``
  The Maximum Transmission unit set for the interface.

``int idx == ifc.index``
  The index of the interface as reported by the system.

``string name == ifc.name``
  The name of the interface as reported by the system.

``string hw_adr== ifc.hw_address``
  The hardware address of the interface. That currently only covers NIC MAC
  addresses.



Instance Metamembers
--------------------

``string s = tostring( Net.Interface i )  [__toString]``
  Returns ``string s`` representing ``Net.Interface`` instance.
  ``string s`` contains name and IP4 address (if available).  I twill look
  like *`T.Net.Interface{br-963e6d75be2a(172.19.0.1)}: 0x5577dfa00f50`*,
  meaning the systems name is *br-963e6d75be2a* and in it's network it's
  currently connected as IP4 address `172.19.0.1`

