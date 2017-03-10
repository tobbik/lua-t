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
#include <string.h>               // memset,memcpy

#include "t.h"
#include "t_buf.h"

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
	struct t_buf                           *cpy_buf = t_buf_check_ud( L, -1, 0 );

	lua_remove( L, 1 );    // remove the T.Buffer Class table
	if (NULL != cpy_buf)
	{
		buf = t_buf_create_ud( L, cpy_buf->len );
		memcpy( &(buf->b[0]), &(cpy_buf->b[0]), buf->len );
		return 1;
	}
	if (lua_isinteger( L, -1 ))
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
struct t_buf
*t_buf_create_ud( lua_State *L, size_t n )
{
	struct t_buf  *b;
	size_t         sz;

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct t_buf ) + (n - 1) * sizeof( char );
	b  = (struct t_buf *) lua_newuserdata( L, sz );
	memset( b->b, 0, n * sizeof( char ) );

	b->len = n;
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
struct t_buf
*t_buf_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_BUF_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_BUF_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_buf *) ud;
}


//////////////////////////////////////////////////////////////////////////////////////
// -- helpers used for T.Buffer and T.Buffer.Segment
//
/**--------------------------------------------------------------------------
 * Get the char *buffer from either a T.Buffer or a T.Buffer.Segment or Lua string.
 * \param   L     Lua state.
 * \param   pos   position of userdata on stack.
 * \param  *len   size_t pointer that holds avilable length in char*.
 * \param  *cw    int pointer determining if char* is writable.
 * \lparam  buf  T.Buffer userdata instance.
 *      OR
 * \lparam  seg  T.Buffer.Segment userdata instance.
 *      OR
 * \lparam  b    Lua string.
 * \return  char*  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
char
*t_buf_tolstring( lua_State *L, int pos, size_t *len, int *cw )
{
	struct t_buf     *buf = t_buf_check_ud( L, pos, 0 );
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, pos, 0 );

	if (NULL != buf)
	{
		*len = buf->len;
		*cw  = 1;
		return (char*) &(buf->b[0]);
	}
	else if (NULL != seg)
	{
		*len = seg->len;
		*cw  = 1;
		return seg->b;
	}
	else if (lua_isstring( L, pos))
	{
		*cw = 0;
		return (char*) luaL_checklstring( L, pos, len );
	}
	else
		return NULL;
}


/**--------------------------------------------------------------------------
 * Get the char *buffer from either a T.Buffer or a T.Buffer.Segment or Lua string.
 * \param   L     Lua state.
 * \param   pos   position of userdata on stack.
 * \param  *len   size_t pointer that holds avilable length in char*.
 * \param  *cw    int pointer determining if char* is writable.
 * \lparam  buf  T.Buffer userdata instance.
 *      OR
 * \lparam  seg  T.Buffer.Segment userdata instance.
 *      OR
 * \lparam  b    Lua string.
 * \return  char*  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
char
*t_buf_checklstring( lua_State *L, int pos, size_t *len, int *cw )
{
	char *b = t_buf_tolstring( L, pos, len, cw );
	luaL_argcheck( L, (b != NULL), pos,
		 "`"T_BUF_SEG_TYPE"` or `"T_BUF_SEG_TYPE"` or string expected" );
	return b;
}


/**--------------------------------------------------------------------------
 * Compares content of two T.Buffer.
 * \param   L    Lua state.
 * \param   bA   Char pointer to buffer content.
 * \param   bB   Char pointer to buffer content to compare to.
 * \param   aLen Length of buffer bA.
 * \param   bLen Length of buffer bB.
 * \lreturn bool true if equal otherwise false.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_buf_compare( lua_State *L, char *bA, char *bB, size_t aLen, size_t bLen )
{
	size_t  n;          ///< runner

	if (bA == bB && aLen == bLen )  // points to the same address -> true
	{
		lua_pushboolean( L, 1 );
		return 1;
	}
	if (aLen != bLen)    // different length -> false
	{
		lua_pushboolean( L, 0 );
		return 1;
	}
	else
		for( n=0; n<aLen; n++ )
			if (bA[n] !=  bB[n])
			{
				lua_pushboolean( L, 0 );
				return 1;
			}
	lua_pushboolean( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer content as a hexadecimal string.
 * \param   L       Lua state.
 * \param  *char    Char pointer to buffer content.
 * \param   len     Length of char buffer.
 * \lreturn string  T.Buffer content representation as hexadecimal string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_buf_getHexString( lua_State *L, char *b, size_t len )
{
	char          hex[] = "0123456789ABCDEF";
	size_t        n;
	luaL_Buffer   lB;
	luaL_buffinit( L, &lB );

	for (n=0; n<len; n++)
	{
		luaL_addchar( &lB, hex[ b[n] >> 4 & 0x0F ] );
		luaL_addchar( &lB, hex[ b[n]      & 0x0F ] );
		luaL_addchar( &lB, ' ' );
	}

	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a binary string.
 * \param   L       Lua state.
 * \param  *char    Char pointer to buffer content.
 * \param   len     Length of char buffer.
 * \lreturn string  T.Buffer content representation as binary string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_buf_getBinString( lua_State *L, char *b, size_t len )
{
	size_t       n, x;
	luaL_Buffer  lB;
	luaL_buffinit( L, &lB );

	for (n=0; n<len; n++)
	{
		for (x=NB; x>0; x--)
			luaL_addchar( &lB, ((b[ n ] >> (x-1)) & 0x01) ? '1' : '0' );
		luaL_addchar( &lB, ' ' );
	}

	luaL_pushresult( &lB );
	return 1;
}


//
// ================================= GENERIC LUA API========================
//
/**--------------------------------------------------------------------------
 * Clears the content of T.Buffer.
 * \param   L    Lua state.
 * \lparam  ud   T.Buffer userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_clear( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	memset( buf->b, 0, buf->len * sizeof( char ) );
	return 0;
}


/**--------------------------------------------------------------------------
 * Read a set of chars from T.Buffer at position x for length y.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer userdata instance.
 * \lparam  pos    Position in T.Buffer where reading starts.
 * \lparam  len    How many bytes shall be read from buf.
 * \lreturn val    Lua string.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_read( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	lua_Integer   idx = (lua_isinteger( L, 2 ))
		? luaL_checkinteger( L, 2 )
		: 1;
	lua_Integer   len = (lua_isinteger( L, 3 ))
		? luaL_checkinteger( L, 3 )
		: buf->len+1 - idx;

	luaL_argcheck( L, 1 <= idx && (size_t)idx         <= buf->len, 2, "index out of range" );
	luaL_argcheck( L, 1 <= len && (size_t)(idx+len-1) <= buf->len, 3, "requested length out of range" );

	lua_pushlstring( L, (const char*) &(buf->b[ idx-1 ]), len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Write an set of chars to T.Buffer at position x for length y.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer userdata instance.
 * \lparam  string Lua string to write to T.Buffer.
 * \lparam  pos    Position in T.Buffer where writing starts.
 * \lparam  len    How many bytes of val shall be written to buf.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_write( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	lua_Integer   idx = (lua_isinteger( L, 3 ))
		? luaL_checkinteger( L, 3 )
		: 1;
	size_t        len;

	luaL_checklstring( L, 2, &len );

	luaL_argcheck( L, lua_isnone( L, 4 ) || (lua_isinteger( L, 4 ) && lua_tointeger( L, 4 ) > 0),
		4, "requested Length must be bigger than 0" );
	len = (lua_isinteger( L, 4 )) ? (size_t) lua_tointeger( L, 4 ) : len;

	luaL_argcheck( L, 1 <= idx && (size_t)idx         <= buf->len, 2, "index out of range" );
	luaL_argcheck( L, 1 <= len && (size_t)(idx+len-1) <= buf->len, 3, "requested length out of range" );

	memcpy( (char*) &(buf->b[ idx-1 ]), lua_tostring( L, 2 ), len );
	return 0;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a hexadecimal string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn string  T.Buffer representation as hexadecimal string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_toHexString( lua_State *L )
{
	struct t_buf *buf   = t_buf_check_ud( L, 1, 1 );
	return t_buf_getHexString( L, buf->b, buf->len );
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a binary string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn string  T.Buffer representation as binary string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_toBinString( lua_State *L )
{
	struct t_buf *buf   = t_buf_check_ud( L, 1, 1 );
	return t_buf_getBinString( L, buf->b, buf->len );
}


/**--------------------------------------------------------------------------
 * Compares two T.Buffer.
 * \param   L    Lua state.
 * \lparam  ud   t_buf userdata instance.
 * \lparam  ud   t_buf userdata instance to compare to.
 * \lreturn bool true if equal otherwise false.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__eq( lua_State *L )
{
	struct t_buf *bA = t_buf_check_ud( L, 1, 1 );
	struct t_buf *bB = t_buf_check_ud( L, 2, 1 );

	if (bA == bB)
		lua_pushboolean( L, 1 );
	else
		t_buf_compare( L, bA->b, bB->b, bA->len, bB->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Combines two T.Buffer into a new T.Buffer.
 * \param   L    Lua state.
 * \lparam  ud   T.Buffer userdata instance.
 * \lparam  ud   T.Buffer userdata instance to add.
 * \lreturn ud   T.Buffer userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__add( lua_State *L )
{
	struct t_buf *bA = t_buf_check_ud( L, 1, 1 );
	struct t_buf *bB = t_buf_check_ud( L, 2, 1 );
	struct t_buf *bR = t_buf_create_ud( L, bA->len + bB->len );

	memcpy( (char*) &(bR->b[ 0 ]),       bA->b, bA->len );
	memcpy( (char*) &(bR->b[ bA->len ]), bB->b, bB->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns len of the buffer
 * \param   L    Lua state
 * \lparam  ud   T.Buffer userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__len( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );

	lua_pushinteger( L, (lua_Integer) buf->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a buffer stream.
 * \param   L      Lua state
 * \lparam  ud     T.Buffer userdata instance.
 * \lreturn string Formatted string representing buffer.
 * \return  int    # of values pushed onto the stack.
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
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_cf [] = {
	  { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_buf_m [] = {
	// metamethods
	  { "__tostring"   , lt_buf__tostring }
	, { "__len"        , lt_buf__len }
	, { "__eq"         , lt_buf__eq }
	, { "__add"        , lt_buf__add }
	// instance methods
	, { "clear"        , lt_buf_clear }
	, { "read"         , lt_buf_read }
	, { "write"        , lt_buf_write }
	// universal stuff
	, { "toHex"        , lt_buf_toHexString }
	, { "toBin"        , lt_buf_toBinString }
	, { NULL           , NULL }
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
	luaopen_t_buf_seg( L );
	lua_setfield( L, -2, T_BUF_SEG_NAME );
	luaL_newlib( L, t_buf_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

