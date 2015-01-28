/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_msg.c
 * \brief     OOP wrapper for HTTP Message (incoming or outgoing)
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>               // malloc, free
#include <string.h>               // strchr, ...

#include "t.h"
#include "t_htp.h"


static int lt_htp_msg__gc( lua_State *luaVM );


/**--------------------------------------------------------------------------
 * create a t_htp_msg and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_msg*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_msg
*t_htp_msg_create_ud( lua_State *luaVM, struct t_htp_srv *srv )
{
	struct t_htp_msg *m;
	m = (struct t_htp_msg *) lua_newuserdata( luaVM, sizeof( struct t_htp_msg ));
	m->bRead  = 0;
	m->pS     = T_HTP_STA_ZERO;
	m->mth    = T_HTP_MTH_ILLEGAL;
	m->srv    = srv;
	m->length = 0;
	m->expect = 0;

	luaL_getmetatable( luaVM, "T.Http.Message" );
	lua_setmetatable( luaVM, -2 );
	return m;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_msg struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_msg*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_msg
*t_htp_msg_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Http.Message" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Message` expected" );
	return (struct t_htp_msg *) ud;
}


static void t_htp_msg_adjustbuffer( struct t_htp_msg *m, size_t read, const char* rpos )
{
	memcpy( &(m->buf), rpos, (const char*) &(m->buf) + read - rpos );
}



/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Message socket.
 * Called anytime the client socket returns from the poll for read.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_msg.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_msg_rcv( lua_State *luaVM )
{
	struct t_htp_msg *m   = t_htp_msg_check_ud( luaVM, 1, 1 );
	// struct t_ael     *ael;
	int               rcvd;
	const char       *nxt;   // pointer to the buffer where processing must continue

	// read
	rcvd = t_sck_recv( luaVM, m->sck, &(m->buf[ m->bRead ]), BUFSIZ - m->bRead );
	if (!rcvd)     // peer has closed
		return lt_htp_msg__gc( luaVM );
	else           // get the proxy on the stack to fill out verb/url/version/headers ...
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR ); //S:m,P

	printf( "Received %d  \n'%s'\n", rcvd, &(m->buf[ m->bRead ]) );

	nxt = &(m->buf[ m->bRead ]);

	while (NULL != nxt)
	{
		// TODO: switching should be done on state and function execution
		// never remove reading handle. Eithe keepAlive takes over or the
		// connection gets destroyed on a read of 0 bytes
		switch (m->pS)
		{
			case T_HTP_STA_ZERO:
				nxt = t_htp_pReqFirstLine( luaVM, m, nxt );
			case T_HTP_STA_HEADER:
				nxt = t_htp_pHeaderLine( luaVM, m, nxt );
				break;
			case T_HTP_STA_HEADDONE:
				// execute function from server
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->rR );
				lua_pushvalue( luaVM, 1 );
				lua_call( luaVM, 1,0 );
				// keep reading if body, else stop reading
				if (m->length > 0 )
				{
					m->pS = T_HTP_STA_BODY;
					t_htp_msg_adjustbuffer( m, rcvd, nxt );
					// read body
				}
				else
					nxt = NULL;     // signal while loop to be done
				break;
			case T_HTP_STA_BODY:
				// execute req.onData
				nxt = NULL;
				break;
			case T_HTP_STA_FINISH:
				// ignore
				nxt = NULL;
				break;
			default:
				luaL_error( luaVM, "Illegal state for T.Http.Message %d", (int) m->pS );
		}
	}


	lua_pop( luaVM, 1 );             // pop the proxy table
	return rcvd;
}


/**--------------------------------------------------------------------------
 * Handle outgoing T.Http.Message into it's socket.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_msg.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_msg_rsp( lua_State *luaVM )
{
	struct t_htp_msg *m   = t_htp_msg_check_ud( luaVM, 1, 1 );
	struct t_ael     *ael;
	size_t            len;
	size_t            row, chr;
	const char       *buf;

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // get the proxy on the stack
	lua_pushstring( luaVM, "oBuffer"  );
	lua_rawget( luaVM, -2 );
	lua_pushstring( luaVM, "sendRow"  );
	lua_rawget( luaVM, -3 );
	row = luaL_checkinteger( luaVM, -1 );
	lua_pushstring( luaVM, "sendChar" );
	lua_rawget( luaVM, -4 );
	chr = luaL_checkinteger( luaVM, -1 );
	lua_pop( luaVM, 2 );
	lua_rawgeti( luaVM, -3, row );   // get current buffer
	buf = luaL_checklstring( luaVM, -1, &len );

	chr += t_sck_send( luaVM, m->sck, &(buf[ chr ]), len );

	// S:msg,prx,oBuffer,buf
	if (chr == len ) // if current buffer row didn't get sens completely
	{
		lua_pushstring( luaVM, "sendChar" );
		lua_pushinteger( luaVM, chr );
		lua_rawset( luaVM, -3 );
	}
	else
	{
		if (row == lua_rawlen( luaVM, -2 ) ) // if current buffer was final buffer
		{
			if (T_HTP_STA_FINISH == m->pS)
			{
				// if keepalive reverse socket again and create timeout function
				if (! m->kpAlv)
				{
					lua_pushcfunction( luaVM, lt_htp_msg__gc );
					lua_pushvalue( luaVM, 1 );
					lua_call( luaVM, 1, 0 );
				}
				else     // start reading again
				{
					printf("Revert Socket: Keep-Alive: %d\n", m->kpAlv);
					lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
					m->sent = 0; m->bRead = 0; m->pS=T_HTP_STA_ZERO;
					ael = t_ael_check_ud( luaVM, -1, 1 );
					t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
					t_ael_addhandle_impl( ael, m->sck->fd, T_AEL_RD );
					ael->fd_set[ m->sck->fd ]->t = T_AEL_RD;
					lua_pop( luaVM, 1 );        // pop the loop
				}
			}
			else
			{
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
				ael = t_ael_check_ud( luaVM, -1, 1 );
				t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
				ael->fd_set[ m->sck->fd ]->t = ael->fd_set[ m->sck->fd ]-> t & (~T_AEL_WR);
				lua_pop( luaVM, 1 );        // pop the loop
			}
		}
		else
		{
		}
	}

	return 1;
}


static void
t_htp_msg_prepresp( lua_State *luaVM, struct t_htp_msg *m )
{
	//S: msg,prx,buffer
	lua_pushstring( luaVM, "ResCode" );
	lua_rawget( luaVM, -3 );
	lua_pushstring( luaVM, "ResLength" );
	lua_rawget( luaVM, -4 );
	//S: msg,prx,buffer,code,length

	t_htp_srv_setnow( m->srv, 0 );            // update server time

	lua_pushfstring( luaVM,
		"HTTP/1.1 %d OK\r\n"
		"Connection: %s\r\n"
		"Date: %s\r\n",
		lua_tointeger( luaVM, -2 ),
		(m->kpAlv) ? "Keep-Alive" : "Close",
		m->srv->fnw
	);
	//S: msg,prx,buffer,code,length,str1
	if (lua_isnumber( luaVM, -3 ) )
		lua_pushfstring( luaVM,
			"Content-Length: %d\r\n",
			luaL_checkinteger( luaVM, -3 )
		);
	else
		lua_pushfstring( luaVM,
			"Transfer-Encoding: chunked\r\n"
		);
	lua_pushfstring( luaVM, "\r\n" );
	lua_concat( luaVM, 3 );
	//S: msg,prx,buffer,code,length,str1
	lua_rawseti( luaVM, -4, 1 );     // set result as first element of oBuffer
	lua_pop( luaVM, 2 );
}


/**--------------------------------------------------------------------------
 * Write a response to the T.Http.Message.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_write( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	struct t_ael     *ael;
	size_t            sz;
	luaL_checklstring( luaVM, 2, &sz );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // get the proxy on the stack
	lua_pushstring( luaVM, "oBuffer" );
	if (T_HTP_STA_SEND != m->pS)
	{
		m->sent  = 0;
		t_htp_msg_prepresp( luaVM, m );
		lua_newtable( luaVM );
		lua_pushvalue( luaVM, -1 );
		lua_insert( luaVM, -3 );
		//S: msg,string,prx,buffer,'oBuffer',buffer
		lua_rawset( luaVM, -4 );
	}
	else
		lua_rawget( luaVM, -2 );

	lua_pushvalue( luaVM, 2 );
	lua_rawseti( luaVM, -2, lua_rawlen( luaVM, -2 ) );

	if (T_HTP_STA_SEND != m->pS)
	{
		m->pS = T_HTP_STA_SEND;
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_addhandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_WR;
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Finish of sending the T.Http.Message response.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_finish( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	if (lua_gettop( luaVM ) > 1)
		lt_htp_msg_write( luaVM );
	m->pS = T_HTP_STA_FINISH;

	return 0;
}


/**--------------------------------------------------------------------------
 * Sets the onData method in T.Http.Message.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  function to be executed when body data arrives on connection.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_onbody( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	
	if (lua_isfunction( luaVM, 2 ))
	{
		m->bR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
		return 0;
	}
	if (lua_isnoneornil( luaVM, 2 ))
	{
		m->bR = LUA_NOREF;
		return 0;
	}
	else
		return t_push_error( luaVM, "Argument must be function or nil" );
}


/**--------------------------------------------------------------------------
 * Sets the content-length of Response
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  length
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_setlength( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );

	if (lua_isnumber( luaVM, 2 ))
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // get the proxy on the stack
		lua_pushstring( luaVM, "ResLength" );
		lua_pushvalue( luaVM, 2 );
		lua_rawset( luaVM, -3 );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Message by accessing proxy table.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__index( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, -2, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // fetch the proxy table
	lua_pushvalue( luaVM, -2 );                      // repush the key
	lua_gettable( luaVM, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update  NOT ALLOWED.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__newindex( lua_State *luaVM )
{
	t_htp_msg_check_ud( luaVM, -3, 1 );

	return t_push_error( luaVM, "Can't change values in `T.Http.Message`" );
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_msg  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__tostring( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

	lua_pushfstring( luaVM, "T.Http.Message: %p", m );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   luaVM      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__len( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	lua_pushinteger( luaVM, m->length );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_msg  The Message instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__gc( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	struct t_ael     *ael;

	if (LUA_NOREF != m->pR)
	{
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->pR );
		m->pR = LUA_NOREF;
	}
	if (NULL != m->sck)
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_RD );
		t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_NO;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->rR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->wR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->hR );
		free( ael->fd_set[ m->sck->fd ] );
		ael->fd_set[ m->sck->fd ] = NULL;

		t_sck_close( luaVM, m->sck );
		m->sck = NULL;
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	printf("GC'ed HTTP connection\n");

	return 0;
}



/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_msg_prx_m [] = {
	{"write",        lt_htp_msg_write},
	{"finish",       lt_htp_msg_finish},
	{"onBody",       lt_htp_msg_onbody},
	{"setLength",    lt_htp_msg_setlength},
	{NULL,    NULL}
};



/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_htp_msg( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Message" );
	lua_pushcfunction( luaVM, lt_htp_msg__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_msg__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_htp_msg__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_msg__gc );
	lua_setfield( luaVM, -2, "__gc");
	lua_pushcfunction( luaVM, lt_htp_msg__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	luaL_newmetatable( luaVM, "T.Http.Message.Proxy" );
	luaL_newlib( luaVM, t_htp_msg_prx_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

