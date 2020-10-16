/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP/RAW, read write connect listen bind etc
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t_net_l.h"
#include "t_buf.h"
#include <stdlib.h>   // bsearch()

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \lparam  protocol string: 'tcp', 'udp' ...  -- use lower cased for platform
 *                                                independence
 * \lparam  family   string: 'ip4', 'AF_INET6', 'raw' ...
 * \lparam  type     string: 'stream', 'datagram' ...
 * \usage   Net.Socket( )                   -> create TCP IPv4 Socket
 *          Net.Socket( 'TCP' )             -> create TCP IPv4 Socket
 *          Net.Socket( 'TCP', 'ip4' )      -> create TCP IPv4 Socket
 *          Net.Socket( 'UDP', 'ip4' )      -> create UDP IPv4 Socket
 *          Net.Socket( 'UDP', 'ip6' )      -> create UDP IPv6 Socket
 * \return  struct t_net_sck pointer to the socket struct.
 * --------------------------------------------------------------------------*/
static int
lt_net_sck_New( lua_State *L )
{
	struct t_net_sck *sck;

	sck  = t_net_sck_create_ud( L );
	p_net_sck_createHandle( L, sck,
	   (AF_UNIX == luaL_checkinteger( L, 2 )) ? 0 : lua_tointeger( L, 2 ),
	   luaL_checkinteger( L, 3 ),
	   luaL_checkinteger( L, 1 ) );

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
*t_net_sck_create_ud( lua_State *L )
{
	struct t_net_sck *sck  = (struct t_net_sck *) lua_newuserdata( L, sizeof( struct t_net_sck ) );
	sck->fd = 0;
	luaL_getmetatable( L, T_NET_SCK_TYPE );
	lua_setmetatable( L, -2 );

	return sck;
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
	//printf("CLOSING SOCK: %d\n", sck->fd);
	return p_net_sck_close( L, sck );
}


/** -------------------------------------------------------------------------
 * Shutdown a socket.
 * \param   L    Lua state.
 * \lparam  sck  Net.Socket userdata instance.
 * \return  int  # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_shutDown( lua_State *L )
{
	struct t_net_sck *sck     = t_net_sck_check_ud( L, 1, 1 );
	return p_net_sck_shutDown( L, sck, luaL_checkinteger( L, 2 ) );
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L      Lua state.
 * \lparam  sck    Net.Sck userdata instance( socket ).
 * \lparam  adr    Net.Address userdata instance( ipaddr ).
 * \lparam  int    port to listen on.
 * \lparam  int    Backlog connections.
 * \lreturn sck    t.Net.Socket  userdata(struct).
 * \lreturn adr    t.Net.Address userdata(struct).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_listener( lua_State *L )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 1 );
	int                 bl  = luaL_optinteger( L, 2, SOMAXCONN );

	return p_net_sck_listen( L, sck, bl );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_adr userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_binder( lua_State *L )
{
	struct t_net_sck        *sck = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 2, 1 );

	return p_net_sck_bind( L, sck, adr );
}


/** -------------------------------------------------------------------------
 * Connect an existing socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  adr    userdata;t_net_adr userdata instance.
 *          OR
 * \lparam  family string;
 * \lparam  ipstr  string; string representing IP.
 * \lparam  port   integer; port number.
 * \lreturn adr    userdata; t_net_adr userdata instance (optional).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_connecter( lua_State *L )
{
	struct t_net_sck        *sck = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 2, 1 );

	return p_net_sck_connect( L, sck, adr );
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \lparam  srv    T.Net.Socket  userdata instance( server socket ).
 * \lreturn cli    T.Net.Socket  userdata instance( new client socket ).
 * \lreturn adr    T.Net.Address userdata instance( new client sockaddr ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_accept( lua_State *L )
{
	struct t_net_sck        *srv = t_net_sck_check_ud( L, 1, 1 );// listening socket
	struct t_net_sck        *cli = t_net_sck_create_ud( L );     // accepted socket
	struct sockaddr_storage *adr = t_net_adr_create_ud( L );     // peer address
	return p_net_sck_accept( L, srv, cli, adr );
}


/** -------------------------------------------------------------------------
 * Send data to a socket.
 *
 * A Buffer, Buffer.Segment or Lua string is mandatory as second parameter.  If
 * the third parameter is a Net.Address it will be passed to sendto and used
 * for an unconnected socket to determine where it goes to.  A fourth parameter,
 * or if Net.Address is omitted a third, is an integer and determines the number
 * of bytes to send.  The following permutations are possible:
 *     cnt,err = s:send( buf/seg/str )
 *     cnt,err = s:send( buf/seg/str, adr )
 *     cnt,err = s:send( buf/seg/str, max )
 *     cnt,err = s:send( buf/seg/str, adr, max )
 * \usage   int cnt = sck:send( Buffer/Segment/string buf[, Net.Address adr, int size ] )
 * \param   L      Lua state.
 * \lparam  sck    Net.Socket  userdata instance.       -> mandatory
 * \lparam  msg    Buffer/Segment/string instance.      -> mandatory
 * \lparam  adr    Net.Address userdata instance.       -> optional
 * \lparam  max    size of msg t be send in bytes.      -> optional
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_send( lua_State *L )
{
	size_t                   len; // length of message to send
	ssize_t                  snt; // actually sent bytes
	struct t_net_sck        *sck = t_net_sck_check_ud( L, 1, 1 );
	char                    *msg = t_buf_checklstring( L, 2, &len, NULL );
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 3, 0 );
	size_t                   max = (lua_gettop( L ) == ((NULL==adr) ?3 :4))
	                               ? (size_t) luaL_checkinteger( L, (NULL==adr) ?3 :4 )
	                               : len;

	snt = p_net_sck_send( L, sck, adr, msg, (max<len) ? max : len );
	if (snt > -1)
		lua_pushinteger( L, snt );
	return ((snt < 0) ? 3 : 1);   // for <0 is false, errMsg, errNo on stack
}


/** -------------------------------------------------------------------------
 * Recieve some data from a socket.
 * If the first parameter is a Net.Address it will be passed to recvfrom() and
 * be filled with the peers ip Address.  If the first or second parameter is a
 * Buffer or a Buffer.Segement received data will be written into it.
 * Otherwise a Lua string with the data will be returned as first return value.
 * If a Buffer or Buffer.Segement is passed the receiving of data is
 * automatically capped to the buffers/segements defined length.  It is not
 * possible to pass an offset to Buffer, instead use a temporary Buffer.Segement
 * to compose a bigger Buffer from multiple recv() operations.  The following
 * permutations are possible:
 *   str ,int = sck:recv( )
 *   str ,int = sck:recv( adr )
 *   str ,int = sck:recv( max )
 *   bool,int = sck:recv( buf/seg )
 *   str ,int = sck:recv( adr, max )
 *   bool,int = sck:recv( adr, buf/seg )
 *   bool,int = sck:recv( buf/seg, max )
 *   bool,int = sck:recv( adr, buf/seg, max )
 * \usage   string msg, int cnt = sck:recv( [Net.Address adr, int size ] )
 * \usage   bool rcvd, int cnt  = sck:recv( [Net.Address adr,] Buffer/Segment buf[, int size ] )
 * \param   L      Lua state.
 * \lparam  sck    Net.Socket  userdata instance.       -> mandatory
 * \lparam  adr    Net.Address userdata instance.       -> optional
 * \lparam  buf    Buffer/Segment userdata instance.    -> optional
 * \lparam  int    size of msg t be received in bytes.  -> optional
 * \lreturn msg    Lua string of received message or boolean true if written to Buffer.
 *                 Is nil or false if nothing was received.
 * \lreturn rcvd   number of bytes recieved.  0 if nothing was received.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_recv( lua_State *L )
{
	size_t                   len  = 0;  // length of sink
	size_t                   max;       // desired or determined size to recv
	int                      cw   = 0;  // test buffer to be writeable
	ssize_t                  rcvd = 0;  // actually rcvd bytes
	char                     buf[ BUFSIZ ]; // char buffer if no t.Buffer is passed
	char                    *msg;       // char pointer to recv into
	size_t                   args = lua_gettop( L );
	struct t_net_sck        *sck  = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_storage *adr  = t_net_adr_check_ud( L, 2, 0 );

	if (t_buf_isstring( L, (NULL==adr) ?2 :3, &cw ) && cw)  // is writable -> buffer
	{
		msg = t_buf_checklstring( L, (NULL==adr) ?2 :3, &len, &cw );
		max = (args == ((NULL==adr) ?3 :4)) ? (size_t) luaL_checkinteger( L, (NULL==adr) ?3 :4 ) : len;
		luaL_argcheck( L, max<=len, (NULL==adr) ?2 :3, "max must be smaller than sink" );
		rcvd = p_net_sck_recv( L, sck, adr, msg, max );
		if (rcvd > 0 )  // 0 for nothing received
			lua_pushboolean( L, 1 );
	}
	else
	{
		max = (args == ((NULL==adr) ?2 :3)) ? luaL_checkinteger( L, (NULL==adr) ?2 :3 ) : BUFSIZ-1;
		luaL_argcheck( L, max<BUFSIZ, (NULL==adr) ? 2:3, "max must be smaller than BUFSIZ" );
		rcvd = p_net_sck_recv( L, sck, adr, buf, max );
		if (rcvd > 0 )  // 0 for nothing received, -1 for errno being set
			lua_pushlstring( L, buf, rcvd );
	}
	if (rcvd == 0)
		lua_pushnil( L );
	if ( rcvd > -1 )
		lua_pushinteger( L, (lua_Integer) rcvd );
	return ((rcvd < 0) ? 3 : 2);
}


/** -------------------------------------------------------------------------
 * Recieve t.Net.Address from a (TCP) socket.
 * \param   L      Lua state.
 * \lparam  ud     t.Net.Socket  userdata instance.
 * \lparam  ud     t.Net.Address userdata instance.
 * \lreturn ud     t.Net.Address userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_getsockname( lua_State *L )
{
	struct t_net_sck        *sck = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 2, 0 );

	if (NULL == adr)
		adr = t_net_adr_create_ud( L );

	return p_net_sck_getsockname( L, sck, adr );
}


/** -------------------------------------------------------------------------
 * Systemcall select() for ready sockets.
 * \param   L      Lua state.
 * \lparam  table  Net socket table All sockets to read from.
 * \lparam  table  Net socket table All sockets to write to.
 * \lreturn table  Net.Socket table of sockets ready to read from.
 * \lreturn table  Net.Socket table of sockets ready to write to.
 * \return  int    # of values pushed onto the stack.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_Select( lua_State *L )
{
	fd_set            rfds, wfds;
	struct t_net_sck *sck;
	int               rdyScks, i;
	int               rMax       = p_net_sck_mkFdSet( L, 1, &rfds );
	int               wMax       = p_net_sck_mkFdSet( L, 2, &wfds );

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
			if (FD_ISSET( sck->fd, (1==i) ? &rfds : &wfds ))
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


/**--------------------------------------------------------------------------
 * Prints out the socket.
 * \param   L      Lua state.
 * \lparam  sck    Net.Socket userdata instance.
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


static int t_net_sck_optCompare( const void *needle, const void *haystack )
{
	const char                      *const key   = needle;
	const struct t_net_sck_option   *const value = haystack;

	return strcmp( key, value->name );
}


/** -------------------------------------------------------------------------
 * __index; retrieve socket option values and other attributes
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or function name.
 * \lreturn value  Lua value; socket option/attribute value or instance method
 *                 function.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__index( lua_State *L )
{
	struct t_net_sck         *sck = t_net_sck_check_ud( L, 1, 1 );
	struct t_net_sck_option  *opt = bsearch(
	                                     luaL_checkstring( L, 2 )
	                                   , t_net_sck_options
	                                   , T_NET_SCK_OPTS_MAX
	                                   , sizeof( struct t_net_sck_option )
	                                   , t_net_sck_optCompare
	                                );

	if (NULL == opt)  //S: sck key val
	{
		// in case no socket option was requested, relay functions from the
		// metatable if available (send,recv,accept,bind etc.). Can be nil.
		lua_getmetatable( L, 1 );
		lua_pushvalue( L, 2 );
		lua_gettable( L, -2 );
		return 1;
	}
	else
		return p_net_sck_getSocketOption( L, sck, opt );
}


/** -------------------------------------------------------------------------
 * __index; set socket option values and other attributes
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or function name.
 * \lparam  value  Lua value; socket option/attribute value.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__newindex( lua_State *L )
{
	struct t_net_sck_option  *opt = bsearch(
	                                     luaL_checkstring( L, 2 )
	                                   , t_net_sck_options
	                                   , T_NET_SCK_OPTS_MAX
	                                   , sizeof( struct t_net_sck_option )
	                                   , t_net_sck_optCompare
	                                );

	if (NULL == opt)              //S: sck key val
		return luaL_error( L, "Can't set unknown socket option: `%s`", lua_tostring( L, 2 ) );
	else
	{
		if (opt->set)
			return p_net_sck_setSocketOption( L, t_net_sck_check_ud( L, 1, 1 ), opt );
		else
			return luaL_error( L, "Socket option: `%s` is read-only", lua_tostring( L, 2 ) );
	}
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_sck_fm [] = {
	  { NULL          , NULL                   }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_cf [] =
{
	  { "select"      , lt_net_sck_Select      }
	, { "new"         , lt_net_sck_New         }
	, { NULL          , NULL                   }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_m [] =
{
	// metamethods
	  { "__tostring"  , lt_net_sck__tostring   }
	, { "__index"     , lt_net_sck__index      }
	, { "__newindex"  , lt_net_sck__newindex   }
	, { "__gc"        , lt_net_sck_close       }
	// object methods
	, { "listener"    , lt_net_sck_listener    }
	, { "binder"      , lt_net_sck_binder      }
	, { "connecter"   , lt_net_sck_connecter   }
	, { "accept"      , lt_net_sck_accept      }
	, { "close"       , lt_net_sck_close       }
	, { "shutdowner"  , lt_net_sck_shutDown    }
	, { "send"        , lt_net_sck_send        }
	, { "recv"        , lt_net_sck_recv        }
	, { "getsockname" , lt_net_sck_getsockname }
	, { NULL          , NULL                   }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     Lua state.
 * \lreturn table the library
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_net_sck( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_NET_SCK_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_sck_m, 0 );
	lua_pop( L, 1 );
	p_net_sck_open( );    // native initialization

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_sck_cf );

	lua_pushinteger( L, SHUT_RD );        // No more receptions.
	lua_setfield( L, -2, "SHUT_RD" );
	lua_pushstring( L, "SHUT_RD" );
	lua_rawseti( L, -2, SHUT_RD );
	lua_pushinteger( L, SHUT_WR );        // No more transmissions.
	lua_setfield( L, -2, "SHUT_WR" );
	lua_pushstring( L, "SHUT_WR" );
	lua_rawseti( L, -2, SHUT_WR );
	lua_pushinteger( L, SHUT_RDWR );      // No more receptions or transmissions.
	lua_setfield( L, -2, "SHUT_RDWR" );
	lua_pushstring( L, "SHUT_RDWR" );
	lua_rawseti( L, -2, SHUT_RDWR );

	// Load available Shutdown modes
	luaopen_t_net_sck_typ( L );
	lua_setfield( L, -2, T_NET_SCK_TYP_IDNT );
	// Load available protocols
	luaopen_t_net_sck_ptc( L );
	lua_setfield( L, -2, T_NET_SCK_PTC_IDNT );

	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_sck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
