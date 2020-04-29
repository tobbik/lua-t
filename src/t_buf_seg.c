/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf_seg.c
 * \brief     OOP wrapper for a segment of T.Buffer.Segment
 *            can be used for everything T.Buffer works in
 *            (T.Pack, T.Net.Socket, ...)
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memcpy

#include "t_buf_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/** -------------------------------------------------------------------------
 * Constructor - creates the t.Buffer.Segment instance.
 * \param   L      Lua state.
 * \lparam  CLASS  table Buffer Segment.
 * \lparam  ud     T.Buffer userdata instance.
 * \lparam  idx    Start of Segment. 1-based index in t_buf->b
 * \lparam  len    Length of Segment.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
int
lt_buf_seg__Call( lua_State *L )
{
	struct t_buf     *buf = t_buf_check_ud( L, 2, 1 );
	lua_Integer       idx = luaL_optinteger( L, 3, 1 );
	lua_Integer       len = luaL_optinteger( L, 4, (int) buf->len - idx + 1 );

	while (lua_isinteger( L, -1 ))
		lua_pop( L, 1 );
	//printf( "idx: %lld    LEN: %lld\n", idx, len );
	t_buf_seg_create_ud( L, buf, idx, len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Adjusts T.Buffer.Segment internal values.
 * \param  *L  Lua state.
 *
 * \return struct t_buf_seg*  pointer to the  t_buf_seg struct
 * --------------------------------------------------------------------------*/
static void
t_buf_seg_set( lua_State *L, struct t_buf_seg *seg, struct t_buf *buf, lua_Integer idx, lua_Integer len )
{
	if(! buf)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, seg->bR );
		buf = t_buf_check_ud( L, -1, 1 );
		lua_pop( L, 1 );
	}

	//printf( "%lld    %lld    %ld (%zu  %zu)\n", idx, len, buf->len, seg->idx, seg->len );
	luaL_argcheck( L, 1 <= idx && (size_t) idx         <= buf->len, 1,
	   T_BUF_SEG_TYPE" offset relative to length of "T_BUF_TYPE" out of bound" );
	luaL_argcheck( L, len >= 0 && (size_t) (idx+len-1) <= buf->len, 2,
	   T_BUF_SEG_TYPE" length out of bound" );

	seg->len = (size_t) len;
	seg->idx = (size_t) idx;
	//seg->b   = &(buf->b[ idx-1 ]);
}


/**--------------------------------------------------------------------------
 * Create a T.Buffer.Segment and push to LuaStack.
 * \param  L  Lua state.
 *
 * \return struct t_buf_seg*  pointer to the  t_buf_seg struct
 * --------------------------------------------------------------------------*/
struct t_buf_seg
*t_buf_seg_create_ud( lua_State *L, struct t_buf *buf, lua_Integer idx, lua_Integer len )
{
	struct t_buf_seg  *seg;

	seg = (struct t_buf_seg *) lua_newuserdata( L, sizeof( struct t_buf_seg ) );
	lua_insert( L, -2 );       // move Segment infront of Buffer
	seg->bR = luaL_ref( L, LUA_REGISTRYINDEX );
	t_buf_seg_set( L, seg, buf, idx, len );

	luaL_getmetatable( L, T_BUF_SEG_TYPE );
	lua_setmetatable( L, -2 );
	return seg;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================
//
/**--------------------------------------------------------------------------
 * Return Tostring representation of a buffer segment.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lreturn string Formatted string representing buffer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__tostring( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_BUF_SEG_TYPE"[%d:%d]: %p", seg->idx, seg->len, seg );
	return 1;
}


/**--------------------------------------------------------------------------
 * GC to release reference to t_buf userdata.
 * \param   L    Lua state.
 * \lparam  ud   T.Buffer.Segement userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__gc( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	luaL_unref( L, LUA_REGISTRYINDEX, seg->bR );
	return 0;
}


/**--------------------------------------------------------------------------
 * shift method for Buffer.Segment to shift along buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lparam  val    integer to shift Segment.
 * \lreturn value  boolean if operation was successful.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_shift( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	lua_Integer       val   = luaL_checkinteger( L, 2 );

	t_buf_seg_set( L, seg, NULL, seg->idx + val, seg->len );
	lua_pushboolean( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * next method for Buffer.Segment to shift along buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lreturn value  boolean if operation was successful.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_next( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	struct t_buf     *buf;

	lua_rawgeti( L, LUA_REGISTRYINDEX, seg->bR );
	buf = t_buf_check_ud( L, -1, 1 );
	lua_pop( L, 1 );
	if (seg->idx + seg->len > buf->len)
		lua_pushboolean( L, 0 );
	else
	{
		t_buf_seg_set( L, seg, buf,
			seg->idx + seg->len,
			(seg->idx + 2*seg->len > buf->len)
				? buf->len - seg->idx - seg->len + 1
				: seg->len
		);
		lua_pushboolean( L, 1 );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * __index method for Buffer.Segment.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__index( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	int               idx;
	const char       *key;

	if (lua_isinteger( L, 2 ))
	{
		idx = luaL_checkinteger( L, 2 );
		if (idx < 1 || idx > (int) seg->len)
			lua_pushnil( L );
		else
		{
			key = t_buf_tolstring( L, 1, NULL, NULL );
			lua_pushinteger( L, key[ idx-1 ] );
		}
	}
	else
	{
		key = lua_tolstring( L, 2, NULL );
		if (0 == strncmp( key, "buffer", 6 ))
			lua_rawgeti( L, LUA_REGISTRYINDEX, seg->bR );
		else if (0 == strncmp( key, "start", 5 ) )
			lua_pushinteger( L, seg->idx );
		else if (0 == strncmp( key, "size", 4 ) )
			lua_pushinteger( L, seg->len );
		else if (0 == strncmp( key, "last", 4 ) )
			lua_pushinteger( L, seg->idx + seg->len - 1 );
		else
		{
			lua_getmetatable( L, 1 );  //S: seg key _mt
			lua_pushvalue( L, 2 );     //S: seg key _mt key
			lua_gettable( L, -2 );     //S: seg key _mt key fnc
		}
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * __newindex method for Buffer.Segment.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__newindex( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	int               idx;
	lua_Integer       val = luaL_checkinteger( L, 3 );
	const char       *key;
	char             *b;

	if (lua_isinteger( L, 2 ))
	{
		idx = lua_tointeger( L, 2 );
		luaL_argcheck( L, idx >  0 && idx <= (int) seg->len, 2, "index out of bound" );
		val = luaL_checkinteger( L, 3 );
		// value must be byte sized
		luaL_argcheck( L, val >= 0 && val <=            255, 3, "value out of range" );
		b   = t_buf_tolstring( L, 1, NULL, NULL );
		b[ idx-1 ] = val;
	}
	else
	{
		key = lua_tostring( L, 2 );

		if (0 == strncmp( key, "start", 5 ) )
			t_buf_seg_set( L, seg, NULL, val, seg->len + seg->idx - val );
		else if (0 == strncmp( key, "size", 4 ) )
			t_buf_seg_set( L, seg, NULL, seg->idx, val );
		else if (0 == strncmp( key, "last", 4 ) )
			t_buf_seg_set( L, seg, NULL, seg->idx, val - seg->idx );
		else
			luaL_argerror( L, 2, "Can't set this value in "T_BUF_SEG_TYPE );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_seg_fm [] = {
	  {"__call",        lt_buf_seg__Call}
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_buf_seg_cf [] = {
	  { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_buf_seg_m [] = {
	// metamethods
	  { "__tostring"   , lt_buf_seg__tostring }
	, { "__index"      , lt_buf_seg__index }
	, { "__newindex"   , lt_buf_seg__newindex }
	, { "__len"        , lt_buf__len }
	, { "__eq"         , lt_buf__eq }
	, { "__gc"         , lt_buf_seg__gc }
	// instance methods
	, { "shift"        , lt_buf_seg_shift }
	, { "next"         , lt_buf_seg_next }
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
int
luaopen_t_buf_seg( lua_State *L )
{
	// T.Buffer.Segment instance metatable
	luaL_newmetatable( L, T_BUF_SEG_TYPE );
	luaL_setfuncs( L, t_buf_seg_m, 0 );
	lua_pop( L, 1 );  // balance the stack

	// T.Buffer.Segment class
	luaL_newlib( L, t_buf_seg_cf );
	luaL_newlib( L, t_buf_seg_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

