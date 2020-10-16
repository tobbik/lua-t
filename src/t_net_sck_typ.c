/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck_typ.c
 * \brief     OOP wrapper for network socket type options
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Class library function definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_typ_cf [] =
{
	  { NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket Type options library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_net_sck_typ( lua_State *L )
{
	// reverse setting of typ.SOCK_STREAM=1 AND typ[1]='SOCK_STREAM' allows basic reverse
	// lookup.
	luaL_newlib( L, t_net_sck_typ_cf );
#define DF_TYP( typ )               \
   lua_pushinteger( L, typ );      \
   lua_setfield( L, -2, #typ "" ); \
   lua_pushstring( L, #typ "" );   \
   lua_rawseti( L, -2, typ );

#ifdef SOCK_STREAM
	DF_TYP( SOCK_STREAM );    // 1  Sequenced, reliable, connection-based byte streams.
#endif
#ifdef SOCK_DGRAM
	DF_TYP( SOCK_DGRAM );     // 2  Connectionless, unreliable datagrams of fixed maximum length.
#endif
#ifdef SOCK_RAW
	DF_TYP( SOCK_RAW );       // 3  Raw protocol interface.
#endif
#ifdef SOCK_RDM
	DF_TYP( SOCK_RDM );       // 4  Reliably-delivered messages
#endif
#ifdef SOCK_SEQPACKET
	DF_TYP( SOCK_SEQPACKET ); // 5  Sequenced, reliable, connection-based, datagrams of fixed maximum length.
#endif
#ifdef SOCK_DCCP
	DF_TYP( SOCK_DCCP );      // 6  Datagram Congestion Control Protocol.
#endif
#ifdef SOCK_PACKET
	DF_TYP( SOCK_PACKET );    // 10 Linux specific way of getting packets at the dev level.  For writing rarp and other similar things on the user level.
#endif

 // Flags to be read into the type parameter o}, socket and socketpair and
 // used for the flags parameter of paccept.
#ifdef SOCK_CLOEXEC
	DF_TYP( SOCK_CLOEXEC );   // 02000000  Atomically set close-on-exec flag for the new descriptor(s).
#endif
#ifdef SOCK_NONBLOCK
	DF_TYP( SOCK_NONBLOCK );  // 02000000  Atomically mark descriptor(s) as non-blocking.
#endif

#undef DF_TYP

	return 1;
}

