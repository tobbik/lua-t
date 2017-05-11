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
 * Constructor - creates the T.Buffer.Segment instance.
 * \param   L      Lua state.
 * \lparam  CLASS  table Buffer Segment.
 * \lparam  ud     T.Buffer userdata instance.
 * \lparam  idx    Index in t_buf->b; Start of Segment.
 * \lparam  len    Length of Segment.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_buf_seg__Call( lua_State *L )
{
	size_t            idx;
	size_t            len;
	struct t_buf     *buf = t_buf_check_ud( L, 2, 1 );
	struct t_buf_seg *seg;

	lua_remove( L, 1 );               // remove class table
	idx  = luaL_optinteger( L, 2, 1 ) - 1;
	luaL_argcheck( L, 0<=idx && (size_t)idx <= buf->len, 2,
	   "Offset relative to length of "T_BUF_TYPE" out of bound" );

	len  = luaL_optinteger( L, 3, buf->len-idx );
	luaL_argcheck( L, 0 <= len && (size_t)(idx+len) <= buf->len, 3,
	   T_BUF_SEG_TYPE" length out of bound" );

	while (lua_isinteger( L, -1 ))
		lua_pop( L, 1 );
	seg = t_buf_seg_create_ud( L, buf, idx+1, len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a T.Buffer.Segment and push to LuaStack.
 * \param  L  Lua state.
 *
 * \return struct t_buf_seg*  pointer to the  t_buf_seg struct
 * --------------------------------------------------------------------------*/
struct t_buf_seg
*t_buf_seg_create_ud( lua_State *L, struct t_buf *buf, size_t idx, size_t len )
{
	struct t_buf_seg  *seg;

	seg = (struct t_buf_seg *) lua_newuserdata( L, sizeof( struct t_buf_seg ) );
	seg->idx =  idx;
	seg->len =  len;
	seg->b   = &(buf->b[ idx-1 ]);
	lua_insert( L, -2 );       // move Segment before Buffer
	seg->bR  =  luaL_ref( L, LUA_REGISTRYINDEX );

	luaL_getmetatable( L, T_BUF_SEG_TYPE );
	lua_setmetatable( L, -2 );
	return seg;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================
//
/**--------------------------------------------------------------------------
 * Clears the content of T.Buffer.Segment.
 * \param   L    Lua state.
 * \lparam  ud   T.Buffer.Segment userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_clear( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	memset( seg->b, 0, seg->len * sizeof( char ) );
	return 0;
}


/**--------------------------------------------------------------------------
 * Gets the content of the Buffer.Segment as a hexadecimal string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer.Segment userdata instance.
 * \lreturn string  T.Buffer.Segment representation as hexadecimal string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_toHexString( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	return t_buf_getHexString( L, seg->b, seg->len );
}


/**--------------------------------------------------------------------------
 * Gets the content of the Buffer.Segment as a binary string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer.Segment userdata instance.
 * \lreturn string  T.Buffer.Segment representation as binary string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_toBinString( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	return t_buf_getBinString( L, seg->b, seg->len );
}


/**--------------------------------------------------------------------------
 * Gets the T.Buffer referencing info of this Segment.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn ud      T.Buffer reference.
 * \lreturn idx     Start of Segment in T.Buffer.
 * \lreturn len     Length of this segment.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg_getReferences( lua_State *L )
{
	struct t_buf_seg *seg   = t_buf_seg_check_ud( L, 1, 1 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, seg->bR );
	lua_pushinteger( L, seg->idx );
	lua_pushinteger( L, seg->len );
	return 3;
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
lt_buf_seg__eq( lua_State *L )
{
	struct t_buf_seg*sA = t_buf_seg_check_ud( L, 1, 1 );
	struct t_buf_seg*sB = t_buf_seg_check_ud( L, 2, 1 );

	if (sA == sB)
		lua_pushboolean( L, 1 );
	else
		t_buf_compare( L, sA->b, sB->b, sA->len, sB->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns len of the buffer segment.
 * \param   L    Lua state
 * \lparam  ud   T.Buffer.Segement userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__len( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	lua_pushinteger( L, (lua_Integer) seg->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a buffer stream.
 * \param   L      Lua state.
 * \lparam  ud     T.Buffer.Segement userdata instance.
 * \lreturn string Formatted string representing buffer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_buf_seg__tostring( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_BUF_SEG_TYPE"[%d:%d]: %p", seg->idx, seg->idx+seg->len, seg );
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
	, { "__len"        , lt_buf_seg__len }
	, { "__eq"         , lt_buf_seg__eq }
	, { "__gc"         , lt_buf_seg__gc }
	// instance methods
	, { "clear"        , lt_buf_seg_clear }
	, { "read"         , lt_buf_read }
	, { "write"        , lt_buf_write }
	, { "getBuffer"    , lt_buf_seg_getReferences }
	// universal stuff
	, { "toHex"        , lt_buf_seg_toHexString }
	, { "toBin"        , lt_buf_seg_toBinString }
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
LUAMOD_API int luaopen_t_buf_seg( lua_State *L )
{
	// T.Buffer.Segment instance metatable
	luaL_newmetatable( L, T_BUF_SEG_TYPE );
	luaL_setfuncs( L, t_buf_seg_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Buffer.Segment class
	luaL_newlib( L, t_buf_seg_cf );
	luaL_newlib( L, t_buf_seg_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

