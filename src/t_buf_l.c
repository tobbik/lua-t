/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf_l.c
 * \brief     OOP wrapper for a universal buffer implementation
 *            Allows for reading writing as mutable string
 *            can be used for socket communication
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset,memcpy

#include "t_buf_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

/* translate a relative string position: negative means back from end
 * Translated to macro from Lua-5.4 source code */
#define lua_posrelate( pos, len)    \
	((pos)>=0)                       \
		? (pos)                       \
		: ((len) + pos + 1);


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
	lua_Integer    sz = lua_isinteger( L, -1 ) ? lua_tointeger( L, -1 ) : 0;
	size_t       i_sz = 0;
	struct t_buf *buf = NULL;
	char         *inp = t_buf_tolstring( L, 2, &i_sz, NULL );

	if (lua_isinteger( L, -1 ))
		luaL_argcheck( L, sz > 0, lua_absindex( L, -1 ), T_BUF_TYPE" size must be greater than 0" );
	buf  = t_buf_create_ud( L, (sz) ? sz : i_sz );
	if (i_sz)
		memcpy( &(buf->b[0]), inp,
			(sz > (lua_Integer) i_sz)
				? i_sz
				: (sz)
					? sz
					: i_sz
		);

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

	// size = sizof(...) -1 because the array has already one member
	b  = (struct t_buf *) lua_newuserdata( L, sizeof( struct t_buf ) + (n - 1) * sizeof( char ) );
	memset( b->b, 0, n * sizeof( char ) );

	b->len = n;
	luaL_getmetatable( L, T_BUF_TYPE );
	lua_setmetatable( L, -2 );
	return b;
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
int
lt_buf_clear( lua_State *L )
{
	size_t                   len; // length of buffer/segment
	char                    *buf = t_buf_checklstring( L, 1, &len, NULL );

	memset( buf, 0, len * sizeof( char ) );
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
int
lt_buf_read( lua_State *L )
{
	size_t        bLen;
	char         *buf  = t_buf_checklstring( L, 1, &bLen, NULL );
	lua_Integer start  = lua_posrelate( luaL_optinteger( L, 2,  1 ), (lua_Integer) bLen );
	lua_Integer   len  = luaL_optinteger( L, 3, (lua_Integer) bLen-start+1 );

	// luaL_argcheck( L, cw != 0, 1, "must be "T_BUF_TYPE" or "T_BUF_SEG_TYPE );
	//	printf( "1. S: %lld L: %lld\n", start, len);
	// handle negative len
	if (len<0) { len = -len; start = start - len + 1; }
	//	printf( "2. S: %lld L: %lld\n", start,len);
	// make sure start in range
	if (start<1) { len = len + start - 1; start = 1; }
	//	printf( "3. S: %lld L: %lld\n", start,len);
	if (start+len > (lua_Integer) bLen) len = (lua_Integer) bLen - start + 1;
	//	printf( "4. S: %lld L: %lld\n", start,len);
	if (len>0)
		lua_pushlstring( L, buf + start - 1, len );
	else lua_pushliteral( L, "" );

	return 1;
}


/**--------------------------------------------------------------------------
 * Write a set of bytes to T.Buffer at position x for length y.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer/T.Buffer.Segment userdata instance.
 * \lparam  string Lua string to write to T.Buffer.
 * \lparam  pos    Position in T.Buffer where writing starts.
 * \lparam  len    How many bytes of val shall be written to buf.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_buf_write( lua_State *L )
{
	size_t        bLen;
	size_t        sLen = 0;
	int           cw   = 0;
	char         *buf  = t_buf_checklstring( L, 1, &bLen, &cw );
	lua_Integer   idx  = luaL_optinteger( L, 3, 1 );

	luaL_argcheck( L, cw != 0, 1, "must be "T_BUF_TYPE" or "T_BUF_SEG_TYPE );

	luaL_checklstring( L, 2, &sLen );
	sLen = luaL_optinteger( L, 4, sLen );

	luaL_argcheck( L, 1 <= idx  && (size_t)idx          <= bLen, 2, "index out of range" );
	luaL_argcheck( L, 1 <= sLen && (size_t)(idx+sLen-1) <= bLen, 3, "requested length out of range" );

	memcpy( buf + idx-1, lua_tostring( L, 2 ), sLen );
	return 0;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a hexadecimal string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn string  T.Buffer representation as hexadecimal string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_buf_toHexString( lua_State *L )
{
	size_t                   len; // length of buffer/segment/string
	char                    *buf = t_buf_checklstring( L, 1, &len, NULL );
	size_t                     n;
	char              hexChars[] = "0123456789ABCDEF";
	luaL_Buffer               lB;
	luaL_buffinit( L, &lB );

	for (n=0; n<len; n++)
	{
		luaL_addchar( &lB, hexChars[ buf[n] >> 4 & 0x0F ] );
		luaL_addchar( &lB, hexChars[ buf[n]      & 0x0F ] );
		if (n+1<len)
			luaL_addchar( &lB, ' ' );
	}

	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a binary string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn string  T.Buffer representation as binary string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_buf_toBinString( lua_State *L )
{
	size_t                   len; // length of buffer/segment/string
	char                    *buf = t_buf_checklstring( L, 1, &len, NULL );
	size_t                  n, x;
	luaL_Buffer               lB;
	luaL_buffinit( L, &lB );

	for (n=0; n<len; n++)
	{
		for (x=CHAR_BIT; x>0; x--)
			luaL_addchar( &lB, ((buf[ n ] >> (x-1)) & 0x01) ? '1' : '0' );
		luaL_addchar( &lB, ' ' );
	}

	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Compares two T.Buffer.
 * \param   L    Lua state.
 * \lparam  ud   t_buf userdata instance.
 * \lparam  ud   t_buf userdata instance to compare to.
 * \lreturn bool true if equal otherwise false.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_buf__eq( lua_State *L )
{
	size_t                   lenA; // length of buffer/segment
	char                    *bufA = t_buf_checklstring( L, 1, &lenA, NULL );
	size_t                   lenB; // length of buffer/segment
	char                    *bufB = t_buf_checklstring( L, 2, &lenB, NULL );

	if (lenA != lenB || 0 != strncmp( bufA, bufB, lenA ) )  // different length        -> false
		lua_pushboolean( L, 0 );
	else
		lua_pushboolean( L, 1 );
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
int
lt_buf__len( lua_State *L )
{
	size_t                   len; // length of buffer/segment
	char __attribute__ ((unused)) *buf = t_buf_checklstring( L, 1, &len, NULL );

	lua_pushinteger( L, (lua_Integer) len );
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
 * __index method for t.Buffer.
 * \param   L      Lua state.
 * \lparam  ud     t.Buffer userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__index( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	int           idx;

	if (lua_isinteger( L, 2 ))
	{
		idx = lua_tointeger( L, 2 );
		if (idx < 1 || idx > (int) buf->len)
			lua_pushnil( L );
		else
			lua_pushinteger( L, buf->b[ idx-1 ] );
	}
	else
	{
		lua_getmetatable( L, 1 );  //S: buf key _mt
		lua_pushvalue( L, 2 );     //S: buf key _mt key
		lua_gettable( L, -2 );     //S: buf key val
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * __newndex method for Buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf__newindex( lua_State *L )
{
	struct t_buf *buf = t_buf_check_ud( L, 1, 1 );
	int           idx = luaL_checkinteger( L, 2 );
	int           val = luaL_checkinteger( L, 3 );

	luaL_argcheck( L, val >= 0 && idx <= 255, 3, "value out of range" );
	luaL_argcheck( L, idx >  0 && idx <= (int) buf->len, 2, "index out of bound" );
	buf->b[ idx ] = val;
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_fm [] = {
	  { "__call"       , lt_buf__Call }
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
	, { "__index"      , lt_buf__index }
	, { "__newindex"   , lt_buf__newindex }
	, { "__len"        , lt_buf__len }
	, { "__eq"         , lt_buf__eq }
	, { "__add"        , lt_buf__add }
	// instance methods
	, { "clear"        , lt_buf_clear }
	, { "read"         , lt_buf_read }
	, { "write"        , lt_buf_write }
	// universal stuff/helpers
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
int
luaopen_t_buf( lua_State *L )
{
	// T.Buffer instance metatable
	luaL_newmetatable( L, T_BUF_TYPE );
	luaL_setfuncs( L, t_buf_m, 0 );
	luaopen_t_buf_seg( L );
	lua_setfield( L, -2, T_BUF_SEG_NAME );

	// T.Buffer class
	luaL_newlib( L, t_buf_cf );
	lua_pushinteger( L, BUFSIZ );
	lua_setfield( L, -2, "Size" );
	luaL_newlib( L, t_buf_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

