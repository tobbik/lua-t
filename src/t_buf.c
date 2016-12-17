/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf.c
 * \brief     OOP wrapper for a universal buffer implementation
 *            Allows for reading writing as mutable string
 *            can be used for socket communication
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset

#include "t.h"
#include "t_buf.h"


// --------------------------------- HELPERS Functions

/** -------------------------------------------------------------------------
 * Helper to check arguments for being a t_buf and a valid position.
 * \param   L     Lua state.
 * \param   pB    position on stack which is buffer.
 * \param   pB    position on stack which is position (buffer index).
 *                handled as pointer and decremented by one to deal with the
 *                fact that C char buffers indexes are zero based.
 * \return *t_buf pointer to validated buffer.
 *  -------------------------------------------------------------------------*/

struct
t_buf * t_buf_getbuffer( lua_State *L, int pB, int pP, int *pos )
{
	struct t_buf *buf = t_buf_check_ud( L, pB, 1 );

	*pos = (lua_isnone( L, pP ))
		? 1
		: luaL_checkinteger( L, pP );   ///< starting byte  b->b[pos]

	luaL_argcheck( L,  1 <= *pos && *pos <= (int) buf->len, pP,
		                 "T.Buffer position must be >= 1 or <= #buffer" );
	*pos = *pos-1;     /// account for char array access being 0 based
	return buf;
}



/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////

/** -------------------------------------------------------------------------
 * Constructor - creates the buffer.
 * \param   L      Lua state.
 * \lparam  CLASS  table Buffer.
 * \lparam  int    length of buffer.
 *        ALTERNATIVE
 * \lparam  string buffer content initialized.
 *        ALTERNATIVE
 * \lparam  ud     T.Buffer userdata instance to clone.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_buf__Call( lua_State *L )
{
	size_t                                  sz;
	struct t_buf  __attribute__ ((unused)) *buf;
	struct t_buf                           *cpy_buf = t_buf_check_ud( L, -1, 0);

	lua_remove( L, 1 );    // remove the T.Buffer Class table
	if (NULL != cpy_buf)
	{
		buf = t_buf_create_ud( L, cpy_buf->len );
		memcpy( &(buf->b[0]), &(cpy_buf->b[0]), buf->len );
		return 1;
	}
	if (lua_isnumber( L, -1 ))
	{
		sz  = luaL_checkinteger( L, -1 );
		buf = t_buf_create_ud( L, sz );
	}
	else if (lua_isstring( L, -1 ))
	{
		luaL_checklstring( L, -1, &sz);
		buf = t_buf_create_ud( L, sz );
		memcpy( (char*) &(buf->b[0]), luaL_checklstring( L, -2, NULL ), sz );
	}
	else
	{
		t_push_error( L, "can't create "T_BUF_TYPE" because of wrong argument type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a t_buf and push to LuaStack.
 * \param  L  The lua state.
 *
 * \return struct t_buf*  pointer to the  t_buf struct
 * --------------------------------------------------------------------------*/
struct
t_buf *t_buf_create_ud( lua_State *L, int size )
{
	struct t_buf  *b;
	size_t          sz;

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct t_buf ) + (size - 1) * sizeof( unsigned char );
	b  = (struct t_buf *) lua_newuserdata( L, sz );
	memset( b->b, 0, size * sizeof( unsigned char ) );

	b->len = size;
	luaL_getmetatable( L, T_BUF_TYPE );
	lua_setmetatable( L, -2 );
	return b;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_buf struct and return it
 * \param  L    the Lua State
 * \param  pos  position on the stack
 *
 * \return struct t_buf* pointer to t_buf struct
 * --------------------------------------------------------------------------*/
struct
t_buf *t_buf_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_BUF_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_BUF_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_buf *) ud;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================

/**--------------------------------------------------------------------------
 * Reads a value, unpacks it and pushes it onto the Lua stack.
 * \param   L      lua Virtual Machine.
 * \lparam  struct t_pack.
 * \lreturn value  unpacked value according to packer format.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_buf_unpack( lua_State *L )
{
	int           pos;                               ///< starting byte  b->b[pos]
	struct t_buf *buf;
	struct t_pck *pc;
	size_t        n = 0,j;

	buf = t_buf_getbuffer( L, 1 , 3, &pos );
	pc  = t_pck_getpck( L, 2, &n );
	t_pcr__callread( L, pc, buf->b + pos );

	if (T_PCK_SEQ == pc->t)
	{
		n = lua_rawlen( L, -1 );
		for (j=1; j<=n; j++)
		{
			lua_rawgeti( L, 0-j, j );
		}
		return n;
	}
	else
	{
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Read a set of chars to the buffer at position x.
 * \lparam  buf  userdata of type T.Buffer (struct t_buf).
 * \lparam  pos  position in bytes.
 * \lparam  sz   size in bytes(1-#buf).
 * \lreturn val  lua_String.
 * TODO: check buffer length vs requested size and offset
 *
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_read( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	int           pos;
	int           sz;

	pos = (lua_isnumber( L, 2 )) ? (size_t) luaL_checkinteger( L, 2 ) : 0;
	sz  = (lua_isnumber( L, 3 )) ? (size_t) luaL_checkinteger( L, 3 ) : buf->len - pos;

	lua_pushlstring( L, (const char*) &(buf->b[ pos ]), sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Write an set of chars to the buffer at position x.
 * \lparam  buf  userdata of type T.Buffer (struct t_buf).
 * \lparam  val  lua_String.
 * \lparam  pos  position in bytes.
 * \lparam  sz   size in bits (1-64).
 * TODO: check string vs buffer length
 *
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_write( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	int           pos = (lua_isnumber( L, 3 )) ? luaL_checkinteger( L, 3 ) : 0;
	size_t        sz;

	// if a third parameter is given write only x bytes of the input string to the buffer
	if (lua_isnumber( L, 4 ))
	{
		sz  = luaL_checkinteger( L, 4 );
		memcpy  ( (char*) &(buf->b[ pos ]), luaL_checklstring( L, 2, NULL ), sz );
	}
	// otherwise write the whole thing
	else
		memcpy  ( (char*) &(buf->b[ pos ]), luaL_checklstring( L, 2, &sz ), sz );
	return 0;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer in Hex
 * \lreturn  string buffer representation in Hexadecimal
 *
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_tohexstring( lua_State *L )
{
	int           l, c;
	char         *sbuf;
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );

	sbuf = malloc( 3 * buf->len * sizeof( char ) + 1 );
	memset( sbuf, 0, 3 * buf->len * sizeof( char ) );

	c = 0;
	for (l=0; l < (int) buf->len; l++)
		c += snprintf( sbuf+c, 4, "%02X ", buf->b[l] );

	lua_pushlstring( L, sbuf, c );
	free( sbuf );
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns len of the buffer
 * \param   L    The Lua state
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__len( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );

	lua_pushinteger( L, (int) buf->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a buffer stream.
 * \param   L     The lua state.
 * \lparam  sockaddr  the buffer-Stream in user_data.
 * \lreturn string    formatted string representing buffer.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__tostring( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_BUF_TYPE"[%d]: %p", buf->len, buf );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_fm [] = {
	  {"__call",        lt_buf__Call}
	, {NULL,            NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_cf [] = {
	{NULL,            NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_buf_m [] = {
	// metamethods
	  { "__tostring", lt_buf__tostring }
	, { "__len",      lt_buf__len }
	// instance methods
	, { "unpack",      lt_buf_unpack }
	, { "read",        lt_buf_read }
	, { "write",       lt_buf_write }
	// univeral stuff
	, { "toHex",       lt_buf_tohexstring }
	, { NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * Pushes this library onto the stack.
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_t_buf( lua_State *L )
{
	// T.Buffer instance metatable
	luaL_newmetatable( L, T_BUF_TYPE );
	luaL_setfuncs( L, t_buf_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Buffer class
	luaL_newlib( L, t_buf_cf );
	luaL_newlib( L, t_buf_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

