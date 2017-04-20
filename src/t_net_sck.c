/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP/RAW, read write connect listen bind etc
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t.h"
#include "t_net.h"
#include "t_buf.h"         // the ability to send and recv buffers


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \lparam  protocol string: 'TCP', 'UDP' ...
 * \lparam  family   string: 'ip4', 'ip6', 'raw' ...
 * \lparam  type     string: 'stream', 'datagram' ...
 * \usage   T.Net.Socket( )                   -> create TCP IPv4 Socket
 *          T.Net.Socket( 'TCP' )             -> create TCP IPv4 Socket
 *          T.Net.Socket( 'TCP', 'ip4' )      -> create TCP IPv4 Socket
 *          T.Net.Socket( 'UDP', 'ip4' )      -> create UDP IPv4 Socket
 *          T.Net.Socket( 'UDP', 'ip6' )      -> create UDP IPv6 Socket
 * \return  struct t_net_sck pointer to the socket struct.
 * --------------------------------------------------------------------------*/
static int
lt_net_sck__Call( lua_State *L )
{
	struct t_net_sck *sck;

	lua_remove( L, 1 );         // remove CLASS table

	t_net_getProtocolByName( L, 1, "TCP" );
	t_getTypeByName( L, 2, "AF_INET", t_net_familyList );
	t_getTypeByName( L, 3,
	   (IPPROTO_TCP == luaL_checkinteger( L, 1 ))
	      ? "SOCK_STREAM"
	      : (IPPROTO_UDP == luaL_checkinteger( L, 1 ))
	         ? "SOCK_DGRAM"
	         : "SOCK_RAW",
	   t_net_typeList );

	sck = t_net_sck_create_ud( L,
	   (AF_UNIX==luaL_checkinteger( L, 2 )) ? 0 : lua_tointeger( L, 2 ),
	   luaL_checkinteger( L, 3 ),
	   luaL_checkinteger( L, 1 ),
	   1 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \param   family   int AF_INET, AF_INET6, ...
 * \param   protocol int IPPROTO_UDP, IPPROTO_TCP, ...
 * \param   type     int SOCK_STREAM, SOCK_DGRAM, ...
 * \param   create   bool, should socket be created or just wrapping userdata.
 * \return  struct t_net_sck* pointer to the socket struct.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_create_ud( lua_State *L, int family, int type, int protocol, int create )
{
	struct t_net_sck *sck  = (struct t_net_sck *) lua_newuserdata( L, sizeof( struct t_net_sck ) );

	if (create)
		t_net_sck_createHandle( L, sck, family, type, protocol );
	else
		sck->fd = 0;
	luaL_getmetatable( L, T_NET_SCK_TYPE );
	lua_setmetatable( L, -2 );

	return sck;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net.
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \return  struct t_net_sck*  pointer to the struct t_net_sck.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_SCK_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_NET_SCK_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_net_sck *) ud;
}


/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L    Lua state.
 * \lparam  ud   t_net_sck userdata instance.
 * \return  int  # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_close( lua_State *L )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, 1, 1 );
	return t_net_sck_close( L, sck );
}


/** -------------------------------------------------------------------------
 * Shutdown a socket.
 * \param   L    Lua state.
 * \lparam  ud   t_net_sck userdata instance.
 * \return  int  # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_shutDown( lua_State *L )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, 1, 1 );
	t_getTypeByName( L, 2, "SHUT_RD", t_net_shutList );
	return t_net_sck_shutDown( L, sck, luaL_checkinteger( L, 2 ) );
}


/** -------------------------------------------------------------------------
 * Return the FD int representation of the socket
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_sck_getFd( lua_State *L )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 1 );
	lua_pushinteger( L, sck->fd );
	return 1;
}


/**--------------------------------------------------------------------------
 * Prints out the socket.
 * \param   L      Lua state.
 * \lparam  ud     t_net userdata instance.
 * \lreturn string formatted string representing socket.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_sck__tostring( lua_State *L )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, 1, 1 );

	lua_pushfstring( L, T_NET_SCK_TYPE"{%d}: %p"
		, sck->fd
		, sck );
	return 1;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Sck userdata instance( socket ).
 * \lparam  ud     T.Net.Ip4 userdata instance( ipaddr ).
 * \lparam  int    port to listen on.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_listen( lua_State *L )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 0 );
	struct sockaddr_in *adr = t_net_ip4_check_ud( L, 1+((NULL==sck) ? 0:1), 0 );
	int                 bl  = SOMAXCONN, returnables = 0;

	if (lua_isinteger( L, -1 ) && LUA_TSTRING != lua_type( L, -2 ))
	{
		bl = lua_tointeger( L, -1 );
		lua_pop( L, 1 );
	}
	if (NULL!=sck && 1==lua_gettop( L ))
		; // No address, or host like info given -> assume it's bound already
	else
		returnables += t_net_getdef( L, 1, &sck, &adr );

	return (t_net_sck_listen( L, sck, adr, bl )) ? returnables : 0;
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_bind( lua_State *L )
{
	struct t_net_sck   *sck         = NULL;
	struct sockaddr_in *adr         = NULL;
	int                 returnables = t_net_getdef( L, 1, &sck, &adr );
	return (t_net_sck_bind( L, sck, adr )) ? returnables : 0;
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_connect( lua_State *L )
{
	struct t_net_sck   *sck         = NULL;
	struct sockaddr_in *adr         = NULL;
	int                 returnables = t_net_getdef( L, 1, &sck, &adr );
	return (t_net_sck_connect( L, sck, adr )) ? returnables : 0;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket  userdata instance( server socket ).
 * \lreturn ud     T.Net.Socket  userdata instance( new client socket ).
 * \lreturn ud     T.Net.Address userdata instance( new client sockaddr ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_accept( lua_State *L )
{
	struct t_net_sck   *srv = t_net_sck_check_ud( L, 1, 1 ); // listening socket
	struct t_net_sck   *cli = t_net_sck_create_ud( L, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0 ); // accepted socket
	struct sockaddr_in *adr = t_net_ip4_create_ud( L );      // peer address
	return t_net_sck_accept( L, srv, cli, adr );
}


/** -------------------------------------------------------------------------
 * Send a message.
 *
 * If the first parameter is a T.Net.Address it will be passed to sendto and
 * used to address where it goes.  If the first or second parameter is a
 * T.Buffer or a T.Buffer.Segement received data will be written into it.
 * Otherwise a Lua string with the data will be returned as second return
 * value.  If a T.Buffer or T.Buffer.Segement is passed the receiving of data
 * is automatically capped to the buffers/segements defined length.  It is not
 * possible to pass an offset to T.Buffer, instead use a temporary
 * T.Buffer.Segement to compose a bigger Buffer from multiple recv()
 * operations.  The following permutations are possible:
 *     cnt        = s:send( ip, buf/seg/str )
 *     cnt        = s:send( buf/seg/str )
 *     cnt        = s:send( ip, buf/seg/str, sz )
 *     cnt        = s:send( buf/seg/str, sz )
 * \usage   int cnt = sck:send( [T.Net.Address adr, T.Buffer/Segment buf, int size ] )
 * \param   L      Lua state.
 * \lparam  sck    T.Net.Socket  userdata instance.
 * \lparam  adr    T.Net.Address userdata instance (optional).
 * \lparam  msg    Lua string/T.Buffer/Segement attempting to send.
 * \lparam  sz     number of bytes to send.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_send( lua_State *L )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *adr = t_net_ip4_check_ud( L, 2, 0 );
	int                 snt;
	size_t              len;
	char               *msg = t_buf_tolstring( L, (NULL==adr)?2:3, &len, NULL );
	size_t              sz  = (lua_isinteger( L, -1 ))
	                           ? lua_tointeger( L, -1 )
	                           : len;

	snt = t_net_sck_send( L, sck, adr, msg, (sz>len) ? len : sz );
	if (0==snt)
		lua_pushnil( L );
	else
		lua_pushinteger( L, snt );
	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a socket.
 * If the first parameter is a T.Net.Address it will be passed to recvfrom and
 * be filled with the peers ip Address.  If the first or second parameter is a
 * T.Buffer or a T.Buffer.Segement received data will be written into it.
 * Otherwise a Lua string with the data will be returned as second return
 * value.  If a T.Buffer or T.Buffer.Segement is passed the receiving of data
 * is automatically capped to the buffers/segements defined length.  It is not
 * possible to pass an offset to T.Buffer, instead use a temporary
 * T.Buffer.Segement to compose a bigger Buffer from multiple recv()
 * operations.  The following permutations are possible:
 *      bool,cnt  =  s:recv( ip, buf/seg )
 *      bool,cnt  =  s:recv( buf/seg )
 *      str ,cnt  =  s:recv( ip )
 *      str ,cnt  =  s:recv( )
 *      bool,cnt  =  s:recv( ip, buf/seg, sz )
 *      bool,cnt  =  s:recv( buf/seg, sz )
 *      str ,cnt  =  s:recv( ip, sz )
 *      str ,cnt  =  s:recv( sz )
 * \usage   string msg, int cnt = sck:recv( [T.Net.Address adr, int size ] )
 * \usage   bool rcvd, int cnt  = sck:recv( [T.Net.Address adr,] T.Buffer/Segment buf[, int size ] )
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lreturn rcvd   number of bytes recieved.  nil if nothing was received.
 * \lreturn msg    Lua string of received message.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_recv( lua_State *L )
{
	struct t_net_sck   *sck  = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *adr  = t_net_ip4_check_ud( L, 2, 0 );
	int                 rcvd;
	size_t              len  = 0;  // length of sink
	int                 psh  = 0;
	char               *msg  = t_buf_tolstring( L, (NULL==adr)?2:3, &len, NULL );
	size_t              sz;

	if (NULL == msg)
	{
		char buffer[ BUFSIZ ];
		msg = &(buffer[0]);
		len = sizeof( buffer )-1;
		psh = 1;
	}
	sz = (lua_isinteger( L, -1 )) ? lua_tointeger( L, -1 ) : len;

	luaL_argcheck( L, sz<=len, (NULL==adr) ? 2:3, "size must be smaller than message" );

	rcvd = t_net_sck_recv( L, sck, adr, msg, (sz>len) ? len : sz );

	// push message/nil, length
	if (psh)
	{
		if (0 == rcvd)
			lua_pushnil( L );
		else
			lua_pushlstring( L, msg, rcvd );
		lua_pushinteger( L, rcvd );
	}
	else
	{
		lua_pushboolean( L, 0 != rcvd );
		lua_pushinteger( L, rcvd );
	}
	return 2;
}


/** -------------------------------------------------------------------------
 * Recieve t.Net.Address from a (TCP) socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket  userdata instance.
 * \lparam  ud     T.Net.Address userdata instance.
 * \lreturn ud     T.Net.Address userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_getsockname( lua_State *L )
{
	struct t_net_sck   *sck         = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *ip          = t_net_ip4_check_ud( L, 2, 0 );

	if (NULL == ip)
		ip = t_net_ip4_create_ud( L );

	if (! t_net_sck_getsockname( sck, ip))
		lua_pushnil( L );
	return 1;  // return no matter what to allow testing for nil
}


/** -------------------------------------------------------------------------
 * Systemcall select() for ready sockets.
 * \param   L      Lua state.
 * \lparam  table  T.Net socket array All sockets to read from.
 * \lparam  table  T.Net socket array All sockets to write to.
 * \lreturn table  T.Net.Socket table of sockets ready to read from.
 * \lreturn table  T.Net.Socket table of sockets ready to write to.
 * \return  int    # of values pushed onto the stack.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_Select( lua_State *L )
{
	fd_set            rfds, wfds;
	struct t_net_sck *sck;
	int               rdyScks, i;
	int               rMax       = t_net_sck_mkFdSet( L, 1, &rfds );
	int               wMax       = t_net_sck_mkFdSet( L, 2, &wfds );

	rdyScks = select(
		(wMax > rMax) ? wMax+1 : rMax+1,
		(-1  != rMax) ? &rfds  : NULL,
		(-1  != wMax) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create read  result table
	lua_createtable( L, 0, 0 );     // create write result table
	for (i=1; i<3; i++)
	{
		lua_pushnil( L );
		while (lua_next( L, i ))
		{
			sck = t_net_sck_check_ud( L, -1, 1 ); //S: rdi wri rdr wrr key sck
			if FD_ISSET( sck->fd, (1==i) ? &rfds : &wfds )
			{
				if (lua_isinteger( L, -2 ))         // append numeric idx
					lua_rawseti( L, i+2, lua_rawlen( L, i+2 )+1 );
				else
				{
					lua_pushvalue( L, -2 );          // reuse key for hash idx
					lua_insert( L, -2 );             //S: rdi wri rdr wrr key key sck
					lua_rawset( L, i+2 );
				}
				if (0 == --rdyScks)
				{
					lua_pop( L, 1 );
					break;
				}
			}
			else
				lua_pop( L, 1 );
		}
	}
	return 2;
}


/** -------------------------------------------------------------------------
 * __index; used to get socket option values
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or fucntion name.
 * \lreturn value  int or bool socket option value or function.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__index( lua_State *L )
{
	struct t_net_sck *sck  = t_net_sck_check_ud( L, 1, 1 );

	lua_pushvalue( L, 2 );   // preserve the key
	t_getTypeByName( L, -1, NULL, t_net_optionList );  //S: sck key val opt

	if (lua_isnil( L, -1 ))
	{
		// in case no socket option was requested, relay functions from the
		// metatable if available (send,recv,accept,bind etc.)
		lua_pop( L, 1 );   // pop the nil
		lua_getmetatable( L, 1 );
		lua_pushvalue( L, 2 );
		lua_gettable( L, -2 );
		return 1;
	}
	else
	{
		return t_net_sck_getSocketOption(
			L,
			sck,
			luaL_checkinteger( L, -1 ),
			lua_tostring( L, 2 ) );
	}
}


/** -------------------------------------------------------------------------
 * __newindex; used to get socket option values
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or function name.
 * \lparam  value  int or bool socket option value.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__newindex( lua_State *L )
{
	lua_pushvalue( L, 2 );
	t_getTypeByName( L, 4, NULL, t_net_optionList );  //S: sck key val opt
	if (lua_isnil( L, 4 ))
		return luaL_error( L, "unknown socket option: %s", lua_tostring( L, 2 ) );

	return t_net_sck_setSocketOption(
		L,
		t_net_sck_check_ud( L, 1, 1 ),
		lua_tointeger( L, 4 ),
		lua_tostring( L, 2 ),
		(lua_isboolean( L, 3 ))
			? lua_toboolean( L, 3 )
			: luaL_checkinteger( L, 3 )
	);
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_sck_fm [] = {
	  { "__call"      , lt_net_sck__Call }
	, { NULL          , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_cf [] =
{
	  { "select"      , lt_net_sck_Select }
	, { "bind"        , lt_net_sck_bind }
	, { "connect"     , lt_net_sck_connect }
	, { "listen"      , lt_net_sck_listen }
	, { NULL          , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_m [] =
{
	// metamethods
	  { "__tostring"  , lt_net_sck__tostring }
	, { "__index"     , lt_net_sck__index }
	, { "__newindex"  , lt_net_sck__newindex }
	, { "__gc"        , lt_net_sck_close }
	// object methods
	, { "listen"      , lt_net_sck_listen }
	, { "bind"        , lt_net_sck_bind }
	, { "connect"     , lt_net_sck_connect }
	, { "accept"      , lt_net_sck_accept }
	, { "close"       , lt_net_sck_close }
	, { "shutdown"    , lt_net_sck_shutDown }
	, { "send"        , lt_net_sck_send }
	, { "recv"        , lt_net_sck_recv }
	, { "getsockname" , lt_net_sck_getsockname }
	// generic net functions -> reuse functions
	, { "getFd"       , lt_net_sck_getFd }
	//, { "setOption",   lt_net_setoption }
	, { NULL          , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     Lua state.
 * \lreturn table the library
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net_sck( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_NET_SCK_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_sck_m, 0 );
	lua_pop( L, 1 );

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_sck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
