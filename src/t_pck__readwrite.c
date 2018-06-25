/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck__readwrite.c
 * \brief     control read and write operations on the binary blob
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */
#include <string.h>     // memcpy

#include "t_pck_l.h"
#include "t_buf.h"
#include "t.h"          // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Copy byte by byte from one buffer to another. Honours endianess.
 * \param   dst        char* to write to.
 * \param   src        char* to read from.
 * \param   sz         how many bytes to copy.
 * \param   is_little  treat input as little endian?
 * --------------------------------------------------------------------------*/
static void
t_pck_copyBytes( char *dst, const char *src, size_t sz, int is_little )
{
#ifdef IS_LITTLE_ENDIAN
	if (is_little)
		while (sz-- != 0)
			*(dst++) = *(src+sz);
	else
	{
		src = src+sz-1;
		while (sz-- != 0)
			*(dst++) = *(src-sz);
	}
#else
	int i = 0;
	if (is_little)
		while (sz-- != 0)
			dst[i++] = src[sz];
	else
		while (sz-- != 0)
			dst[MXINT-1-i++] = src[sz];
#endif
}


/**--------------------------------------------------------------------------
 * Get integer value from buffer and pushes to Lua stack.
 * \param  *L    Lua state.
 * \param  *b    char* to read from.
 * \param  *p    struct t_pck*.
 * \param   ofs  int; inner byte offset of bits.
 * \lreturn      pushes int value onto stack.
 * --------------------------------------------------------------------------*/
static void
t_pck_getIntValue( lua_State *L, const char *b, struct t_pck *p, size_t ofs )
{
	lua_Unsigned  msk    = 0,
	              val    = 0;
	char         *out    = (char *) &val;
	size_t        bytes;              ///< how many bytes to copy for ALL bits
	size_t        l_shft;             ///< how far left to shift the value

	if (ofs || p->s % NB)             // parse bitwise if not aligned to byte border
	{
		bytes  = ((p->s + ofs - 1) / NB) + 1;
		t_pck_copyBytes( out, b, bytes, 1 );
		l_shft = (MXBIT - bytes*NB + ofs);
		val    = (val << l_shft) >> (MXBIT - p->s);
	}
	else
		t_pck_copyBytes( out, b, p->s/NB, T_PCK_ISBIG( p->m ) );

	if (T_PCK_ISSIGNED( p->m ))       // 2's complement for signed
	{
		msk = (lua_Unsigned) 1  << (p->s - 1);
		lua_pushinteger( L, (lua_Integer) ((val^msk) - msk) );
	}
	else
		lua_pushinteger( L, val );
}


/**--------------------------------------------------------------------------
 * Read integer value from stack and set to char buffer.
 * \param  *L    Lua state.
 * \param  *b    char* to read from.
 * \param  *p    struct t_pck*.
 * \param   ofs  bit offset for bit type from last byte border (0-7).
 * --------------------------------------------------------------------------*/
static void
t_pck_setIntValue( lua_State *L, char *b, struct t_pck *p, size_t ofs )
{
	lua_Integer  iVal = luaL_checkinteger( L, -1 );
	int     is_signed = (T_PCK_ISSIGNED( p->m ) && p->s < MXBIT);
	int       do_sign = (is_signed && iVal<0 );
	lua_Unsigned  msk = (do_sign) ? (lua_Unsigned) 1  << (p->s - 1) : 0;
	lua_Unsigned  val = (do_sign)
	                    ? (((lua_Unsigned) iVal) + msk) ^ msk
	                    : (lua_Unsigned) iVal;
	size_t          n;

	// if positive and signed, 2's complement can handle only p->s-1 bits wide
	luaL_argcheck( L,  0 == (val >> ((is_signed && ! do_sign) ? p->s-1 : p->s)) ,
	   2, "packed value must fit the value range for the packer size" );
	if (ofs || p->s % NB)
		for (n = p->s; n > 0; n--)
		{
			T_PCK_BIT_SET( *(b + (ofs/NB)), ofs%NB, ((val >> (n-1)) & 0x01) ? 1 : 0 );
			ofs++;
		}
	else
		t_pck_copyBytes( b, (char *) &val, p->s/NB, T_PCK_ISBIG( p->m ) );
}


///////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC t_pck API ========================
// Reader and writer for packer data
/**--------------------------------------------------------------------------
 * reads a value from the packer and pushes it onto the Lua stack.
 * \param  *L      Lua state.
 * \param  *b      const char* buffer to read from (bytes positioned).
 * \param  *p      struct* t_pck.
 * \param   ofs    size_t offset in bit from byte border.
 * \lreturn value  Appropriate Lua value.
 * \return  p->s   amount of bits read from b.
 * -------------------------------------------------------------------------- */
size_t
t_pck_read( lua_State *L, const char *b, struct t_pck *p, size_t ofs )
{
	volatile union Ftypes  u;

	switch( p->t )
	{
		case T_PCK_BOL:
			lua_pushboolean( L, T_PCK_BIT_GET( *b, ofs ) );
			break;
		case T_PCK_INT:
			t_pck_getIntValue( L, b, p, ofs );
			break;
		case T_PCK_FLT:
			luaL_argcheck( L, ofs == 0, 1, "Float Packer must start reading at byte border" );
			t_pck_copyBytes( (char*) &(u), b, p->s/NB, ! p->m );
			if      (sizeof( u.f ) == p->s/NB) lua_pushnumber( L, (lua_Number) u.f );
			else if (sizeof( u.d ) == p->s/NB) lua_pushnumber( L, (lua_Number) u.d );
			else                               lua_pushnumber( L,              u.n );
			break;
		case T_PCK_RAW:
			luaL_argcheck( L, ofs == 0, 1, "Raw Packer must start reading at byte border" );
			lua_pushlstring( L, (const char*) b, p->s/NB );
			break;
		default:
			return luaL_error( L, "Can't read value from unknown packer type" );
	}
	return p->s;
}


/**--------------------------------------------------------------------------
 * Sets a value from stack to a char buffer according to packer format.
 * \param  *L      Lua state.
 * \param  *b      char* buffer to write to (bytes positioned).
 * \param  *p      struct* t_pck.
 * \param   ofs    size_t offset in bit from byte border.
 * \lparam  value  Appropriate Lua value.
 * \return  int    0==success; !=0 errors pushed to Lua stack.
 *  -------------------------------------------------------------------------*/
int
t_pck_write( lua_State *L, char *b, struct t_pck *p, size_t ofs )
{
	volatile union Ftypes  u;
	const char            *strVal;
	size_t                 sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case T_PCK_BOL:
			luaL_argcheck( L,  lua_isboolean( L, -1 ) , -1,
			   "value to pack must be boolean type" );
			T_PCK_BIT_SET( *b, ofs, lua_toboolean( L, -1 ) );
			break;
		case T_PCK_INT:
			t_pck_setIntValue( L, b, p, ofs );
			break;
		case T_PCK_FLT:
			if      (sizeof( u.f ) == p->s/NB) u.f = (float)  luaL_checknumber( L, -1 );
			else if (sizeof( u.d ) == p->s/NB) u.d = (double) luaL_checknumber( L, -1 );
			else                               u.n =          luaL_checknumber( L, -1 );
			t_pck_copyBytes( b, (char*) &(u), p->s/NB, 0 );
			break;
		case T_PCK_RAW:
			strVal = luaL_checklstring( L, -1, &sL );
			luaL_argcheck( L,  p->s/NB >= sL, -1, "String is to big for the field" );
			memcpy( b, strVal, sL );
			break;
		default:
			return luaL_error( L, "Can't pack a value in unknown packer type" );
	}
	return 0;
}

