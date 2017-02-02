/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck.c
 * \brief     OOP wrapper for Packer definitions
 *            Allows for packing/unpacking numeric values to binary streams
 *            can work stand alone or as helper for Combinators
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */
#include <string.h>     // memcpy

#include "t.h"
#include "t_pck.h"

// ========== Buffer accessor Helpers
#define BIT_GET(b,n)       ( ((b) >> (NB-(n)-1)) & 0x01 )
#define BIT_SET(b,n,v)                   \
	( b = ( (1==v)                              \
	 ? ((b) | (  (0x01) << (NB-(n)-1)))   \
	 : ((b) & (~((0x01) << (NB-(n)-1)))) ) )

// global default for T.Pack, can be flipped
#ifdef IS_LITTLE_ENDIAN
static int _default_endian = 1;
#else
static int _default_endian = 0;
#endif


// Function helpers
/**--------------------------------------------------------------------------
 * Copy byte by byte from one string to another. Honours endianess.
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
 * Push an integer value to Lua stack.  Handles signing.
 * \param   L            Lua state.
 * \param   lua_Unsigned num value.
 * \lreturn push numeric value onto stack.
 * --------------------------------------------------------------------------*/
static void
t_pck_pushNumValue( lua_State *L, lua_Unsigned val, int sz, int is_signed )
{
	lua_Unsigned msk;

	if (is_signed)
	{
		msk = (lua_Unsigned) 1  << (sz - 1);
		lua_pushinteger( L, (lua_Integer) ((val^msk) - msk) );
	}
	else
		lua_pushinteger( L, val );
}


/**--------------------------------------------------------------------------
 * Get Byte Value from buffer.
 * \param   L          Lua state.
 * \param   b          char* to read from.
 * \param   sz         how many bits to read.
 * \param   is_little  interpret as Little Endian?
 * \param   is_signed  interpret as signed?
 * \lreturn push value onto stack;
 * --------------------------------------------------------------------------*/
static void
t_pck_getByteValue( lua_State *L, const char *b, size_t sz, int is_little, int is_signed )
{
	lua_Unsigned val    = 0;
	char        *out    = (char *) &val;

	t_pck_copyBytes( out, b, sz/NB, ! is_little );
	t_pck_pushNumValue( L, val, sz, is_signed );
}


/**--------------------------------------------------------------------------
 * Get Bit Value from buffer.
 * \param   L          Lua state.
 * \param   b          char* to read from.
 * \param   sz         how many bits to read.
 * \param   ofs        how many bits offset from beginning of byte.
 * \param   is_signed  interpret as signed?
 * \lreturn push value onto stack;
 * --------------------------------------------------------------------------*/
static void t_pck_getBitValue( lua_State *L, const char *b, size_t sz, size_t ofs, int is_signed )
{
	lua_Unsigned val    = 0;
	char        *out    = (char *) &val;
	// TODO: Isn't it enought to shift right by offset and &Mask ... might be cheaper
	size_t       a_byte = ((sz + ofs) / NB) + 1;;     ///< how many bytes to copy for ALL bits
	size_t       l_shft = (MXBIT - a_byte*NB + ofs);; ///< how far left to shift the value

	t_pck_copyBytes( out, b, a_byte, 1 );
	val = (val << l_shft) >> (MXBIT - sz);
	t_pck_pushNumValue( L, val, sz, is_signed );
}


///////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC t_pck API ========================
// Reader and writer for packer data
/**--------------------------------------------------------------------------
 * reads a value from the packer and pushes it onto the Lua stack.
 * \param   L       Lua state.
 * \param   struct* t_pck.
 * \param   char*   const char buffer to read from (already positioned).
 * \param   size_t  offset in bit from byte border.
 * \lreturn value   Appropriate Lua value.
 * \return  int     # of values pushed onto the stack.
 * -------------------------------------------------------------------------- */
int
t_pck_read( lua_State *L, struct t_pck *p, const char *b, size_t ofs )
{
	volatile union Ftypes  u;

	switch( p->t )
	{
		case T_PCK_INT:
			t_pck_getByteValue( L, b, p->s, p->m, 1 );
			break;
		case T_PCK_UNT:
			t_pck_getByteValue( L, b, p->s, p->m, 0 );
			break;
		case T_PCK_BOL:
			lua_pushboolean( L, BIT_GET( *b, ofs ) );
			break;
		case T_PCK_BTS:
			t_pck_getBitValue( L, b, p->s, ofs, 1 );
			break;
		case T_PCK_BTU:
			t_pck_getBitValue( L, b, p->s, ofs, 0 );
			break;
		case T_PCK_FLT:
			t_pck_copyBytes( (char*) &(u), b, p->s/NB, 0 );
			if      (sizeof( u.f ) == p->s/NB) lua_pushnumber( L, (lua_Number) u.f );
			else if (sizeof( u.d ) == p->s/NB) lua_pushnumber( L, (lua_Number) u.d );
			else                               lua_pushnumber( L, u.n );
			break;
		case T_PCK_RAW:
			lua_pushlstring( L, (const char*) b, p->s/NB );
			break;
		default:
			return t_push_error( L, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Set byte value to char buffer.
 * \param   L          Lua state.
 * \param   b          char* to read from.
 * \param   sz         how many bits to read.
 * \param   is_little  interpret as Little Endian?
 * \param   is_signed  interpret as signed?
 * --------------------------------------------------------------------------*/
static void
t_pck_setByteValue( lua_State *L, char *b, size_t sz, int is_little, int is_signed )
{
	lua_Integer  iVal = luaL_checkinteger( L, -1 );
	lua_Unsigned  msk = (is_signed) ? (lua_Unsigned) 1  << (sz - 1) : 0;
	lua_Unsigned  val = (lua_Unsigned) iVal;

	if (is_signed && iVal<0 && sz != MXBIT)
		val = (val + msk) ^ msk;
	luaL_argcheck( L,  0 == (val >> sz) , 2,
	   "value to pack must be smaller than the maximum value for the packer size" );
	t_pck_copyBytes( b, (char *) &val, sz/NB, ! is_little );
}


/**--------------------------------------------------------------------------
 * Write an integer of y bits from to char buffer with offset ofs.
 * \param   L          Lua state.
 * \param  *b          char buffer already on proper initial position.
 * \param   sz         size in bits.
 * \param   ofs        offset in bits (0-7).
 * \param   is_signed  Handle signed value.
 * --------------------------------------------------------------------------*/
static void
t_pck_setBitValue( lua_State *L, char *b, size_t sz, size_t ofs, int is_signed )
{
	lua_Integer  iVal = luaL_checkinteger( L, -1 );
	lua_Unsigned  msk = (is_signed) ? (lua_Unsigned) 1  << (sz - 1) : 0;
	lua_Unsigned  val = (lua_Unsigned) iVal;
	int           n;

	if (is_signed && iVal<0 && sz != MXBIT)
		val = (val + msk) ^ msk;
	luaL_argcheck( L,  0 == (val >> sz) , 2,
	   "value to pack must be smaller than the maximum value for the packer size" );
	for (n=sz; n>0; n--)
	{
		BIT_SET( *(b + (ofs/NB)), ofs%NB, ((val >> (n-1)) & 0x01) ? 1 : 0 );
		ofs++;
	}
}


/**--------------------------------------------------------------------------
 * Sets a value from stack to a char buffer according to packer format.
 * \param   L       Lua state.
 * \param   struct* t_pck.
 * \param   char*   char buffer to write to (already positioned).
 * \lparam  value   Appropriate Lua value.
 * \return integer  0==success; !=0 errors pushed to Lua stack.
 *  -------------------------------------------------------------------------*/
int
t_pck_write( lua_State *L, struct t_pck *p, char *b, size_t ofs )
{
	volatile union Ftypes  u;
	const char            *strVal;
	size_t                 sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case T_PCK_INT:
			t_pck_setByteValue( L, b, p->s, p->m, 1 );
			break;
		case T_PCK_UNT:
			t_pck_setByteValue( L, b, p->s, p->m, 0 );
			break;
		case T_PCK_BOL:
			luaL_argcheck( L,  lua_isboolean( L, -1 ) , -1,
			   "value to pack must be boolean type" );
			BIT_SET( *b, ofs, lua_toboolean( L, -1 ) );
			break;
		case T_PCK_BTS:
			t_pck_setBitValue( L, b, p->s, ofs, 1 );
			break;
		case T_PCK_BTU:
			t_pck_setBitValue( L, b, p->s, ofs, 0 );
			break;
		case T_PCK_FLT:
			if      (sizeof( u.f ) == p->s/NB) u.f = (float)  luaL_checknumber( L, -1 );
			else if (sizeof( u.d ) == p->s/NB) u.d = (double) luaL_checknumber( L, -1 );
			else                            u.n = luaL_checknumber( L, -1 );
			t_pck_copyBytes( b, (char*) &(u), p->s/NB, 0 );
			break;
		case T_PCK_RAW:
			strVal = luaL_checklstring( L, -1, &sL );
			luaL_argcheck( L,  p->s/NB >= sL, -1, "String is to big for the field" );
			memcpy( b, strVal, sL );
			break;
		default:
			return t_push_error( L, "Can't pack a value in unknown packer type" );
	}
	return 0;
}


// #########################################################################
//  _                      _          _
// | |_ _   _ _ __   ___  | |__   ___| |_ __   ___ _ __ ___
// | __| | | | '_ \ / _ \ | '_ \ / _ \ | '_ \ / _ \ '__/ __|
// | |_| |_| | |_) |  __/ | | | |  __/ | |_) |  __/ |  \__ \
//  \__|\__, | .__/ \___| |_| |_|\___|_| .__/ \___|_|  |___/
//      |___/|_|                       |_|
// #########################################################################
/**--------------------------------------------------------------------------
 * __tostring helper that prints the packer type.
 * \param   L       Lua state.
 * \param   enum    t_pck_typ T.Pack type.
 * \param   size_t  length of packer in bits or bytes (type depending).
 * \param   int     modifier (packer type dependent).
 * \lreturn string  string describing packer.
 * --------------------------------------------------------------------------*/
void
t_pck_format( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	lua_pushfstring( L, "%s", t_pck_t_lst[ t ] );
	switch( t )
	{
		case T_PCK_INT:
		case T_PCK_UNT:
			lua_pushfstring( L, "%d%c", s/NB, (1==m) ? 'L' : 'B' );
			break;
		case T_PCK_FLT:
			lua_pushfstring( L, "%d", s/NB );
			break;
		case T_PCK_BOL:
			lua_pushfstring( L, "%d", m );
			break;
		case T_PCK_BTS:
		case T_PCK_BTU:
			lua_pushfstring( L, "%d:%d", s, m );
			break;
		case T_PCK_RAW:
			lua_pushfstring( L, "%d", s/NB );
			break;
		case T_PCK_ARR:
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_pushfstring( L, "[%d](%d)", s, m );
			break;
		default:
			lua_pushfstring( L, "UNKNOWN" );
	}
	lua_concat( L, 2 );
}


/**--------------------------------------------------------------------------
 * Create userdata struct for T.Pack.
 * Checks first if requested type already exists exists in T.Pack.  Otherwise
 * create and register a new Type.  The format for a particular definition will
 * never change.  Hence there is no need to create multiple copies.  This
 * approach saves memory.
 * \param   L      Lua state.
 * \param   enum   t_pck_t.
 * \param   size   number of elements.
 * \param   mod    parameter.
 * \return  struct t_pck* pointer. Create a t_pack and push to LuaStack.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_create_ud( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	struct t_pck  __attribute__ ((unused)) *p;
	int                                     i;

	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( L, -1, "t" );
	lua_getfield( L, -1, T_PCK_NAME );
	t_pck_format( L, t, s, m );
	lua_rawget( L, -2 );           //S: _ld t t.pck pck/nil
	if (t<T_PCK_ARR || lua_isnil( L, -1 ))        // haven't found in cache -> create it
	{
		p = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ));
		p->t = t;
		p->s = s;
		p->m = m;

		luaL_getmetatable( L, T_PCK_TYPE );
		lua_setmetatable( L, -2 );
		if (t<T_PCK_ARR)        // register atomic types only
		{
			lua_remove( L, -2 );      // remove the nil
			t_pck_format( L, t, s, m );
			lua_pushvalue( L, -2 );   //S: _ld t t.pck pck fmt pck
			lua_rawset( L, -4 );
		}
	}
	p = t_pck_check_ud( L, -1, 1 ); //S: _ld t t.pck pck
	for (i=0; i<3; i++)
		lua_remove( L, -2 );

	return p;                       //S: pck
}


/**--------------------------------------------------------------------------
 * Check if value on stack is T.Pack OR * T.Pack.Struct/Sequence/Array
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \param   int    check -> treats as check -> error if fail
 * \lparam  ud     T.Pack/Struct on the stack.
 * \return  t_pck* pointer to t_pck.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_PCK_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_PCK_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_pck *) ud;
}


/**--------------------------------------------------------------------------
 * Get the size of a packer of any type in bytes.
 * This will mainly be needed to calculate offsets when reading.
 * \param   L       Lua state.
 * \param   struct* t_pck.
 * \return  int     size in bytes.
 * TODO: return 0 no matter if even one item is of unknown length.
 * --------------------------------------------------------------------------*/
size_t
t_pck_getSize( lua_State *L, struct t_pck *p )
{
	size_t        s;         ///< size accumulator for complex types
	size_t        n;         ///< iterator over complex types

	if (p->t < T_PCK_ARR)
		return p->s;
	else
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, p->m ); // get packer or table

		switch (p->t)
		{
			case T_PCK_ARR:
				s = p->s * t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
				break;
			case T_PCK_SEQ:
				s = 0;
				for (n = 1; n <= p->s; n++)
				{
					lua_rawgeti( L, -1, n );    // get packer from table
					t_pck_fld_getPackFromStack( L, -1, NULL );
					s += t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
					lua_pop( L, 1 );
				}
				break;
			case T_PCK_STR:
				s = 0;
				for (n = 1; n <= p->s; n++)
				{
					//t_stackDump(L);
					lua_rawgeti( L, -1, n );
					lua_rawget( L, -2 );
					t_pck_fld_getPackFromStack( L, -1, NULL );
					s += t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
					lua_pop( L, 1 );
				}
				break;
			default:
				s = p->s;
		}
		lua_pop( L, 1 );                           // pop packer or table
	}
	return s;
}


/**--------------------------------------------------------------------------
 * Get T.Pack from a stack element at specified position.
 * Return pointer to the t_pck_fld* if the found element is actually a field
 * and the return of the field pointer is requested by passing a pointer instead
 * of NULL.  If the element at the position is actually a T.Pack.Field it will
 * get replaced on the stack with the referenced T.Pack instance.
 * \param  L        Lua state.
 * \param  int      position on Lua stack.
 * \param  struct** pointer to t_pck_fld pointer.
 * \return struct*  pointer to t_pck.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_fld_getPackFromStack( lua_State * L, int pos, struct t_pck_fld **pcf )
{
	void             *ud = luaL_testudata( L, pos, T_PCK_FLD_TYPE );
	struct t_pck_fld *pf = (NULL == ud) ? NULL : (struct t_pck_fld *) ud;
	struct t_pck     *pc;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	if (NULL == pf)
		return t_pck_check_ud( L, pos, 1 );
	else
	{
		if (NULL != pcf)
			//*pcf = *(&pf);
			*pcf = pf;
		lua_rawgeti( L, LUA_REGISTRYINDEX, pf->pR ); //S:… pf … pc
		pc = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );                       //S:… pc …
		return pc;
	}
}


/**--------------------------------------------------------------------------
 * Decides if the element on pos is a packer kind of type.
 * It decides between the following options:
 *     - T.Pack type              : just return it
 *     - T.Pack.Field             : return reference packer
 *     - fmt string of single item: fetch from cache or create
 *     - fmt string of mult items : let Sequence constructor handle and return result
 * \param   L      Lua state.
 * \param   pos    position on stack.
 * \param   int*   bit offset runner.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getPacker( lua_State *L, int pos, size_t *bo )
{
	struct t_pck *p = NULL; ///< packer
	int           l = _default_endian;
	int           n = 0;  ///< counter for packers created from fmt string
	int           t = lua_gettop( L );  ///< top of stack before operations
	const char   *fmt;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	// if it is a T.Pack or T.Pack.Field
	if (lua_isuserdata( L, pos ))
	{
		p    = t_pck_fld_getPackFromStack( L, pos, NULL );
		// This fixes Bitwise offsets
		if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
		{
			p = t_pck_create_ud( L, p->t, p->s, *bo%NB );
			lua_replace( L, pos );
		}
		*bo += t_pck_getSize( L, p );
	}
	else // if it is a format string
	{
		fmt = luaL_checkstring( L, pos );
		p   = t_pck_fmt_read( L, &fmt, &l, bo );
		while (NULL != p )
		{
			n++;
			p = t_pck_fmt_read( L, &fmt, &l, bo );
		}
		// TODO: actually create the packers and calculate positions
		if (n > 1)
			p =  t_pck_seq_create( L, t+1, lua_gettop( L ), bo );
		else
			p = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );
	}
	return p;
}


//###########################################################################
//   ____                _                   _
//  / ___|___  _ __  ___| |_ _ __ _   _  ___| |_ ___  _ __
// | |   / _ \| '_ \/ __| __| '__| | | |/ __| __/ _ \| '__|
// | |__| (_) | | | \__ \ |_| |  | |_| | (__| || (_) | |
//  \____\___/|_| |_|___/\__|_|   \__,_|\___|\__\___/|_|
//###########################################################################

/** -------------------------------------------------------------------------
 * Constructor for T.Pack.
 * Behaves differently based on arguments.
 * \param     L      Lua state.
 * \lparam    CLASS  table T.Pack.
 * \lparam    fmt    string.
 *      OR
 * \lparam    tbl,…  {name=T.Pack},… name value pairs.
 *      OR
 * \lparam    T.Pack elements.    copy constructor
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int lt_pck__Call( lua_State *L )
{
	struct t_pck  *p;
	struct t_oht  *oht = t_oht_check_ud( L, 2, 0 );
	size_t         bo  = 0;  // running bit offset

	lua_remove( L, 1 );                              // remove the T.Pack Class table
	if (NULL != oht)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );         //S: oht pck tbl
		lua_remove( L, 1 );
		p = t_pck_str_create( L );                            //S: pck tbl
		return 1;
	}
	if (lua_istable( L, 1 ))
	{
		t_oht_readArguments( L, 1, lua_gettop( L ) );
		p = t_pck_str_create( L );                            //S: pck tbl
		return 1;
	}
	else
	{
		if (2==lua_gettop( L ) && lua_isinteger( L, 2 ))
			p = t_pck_arr_create( L );
		else
			p = t_pck_getPacker( L, 1, &bo );
	}
	return 1;
}


//###########################################################################
//   ____ _                                _   _               _
//  / ___| | __ _ ___ ___   _ __ ___   ___| |_| |__   ___   __| |___
// | |   | |/ _` / __/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
// | |___| | (_| \__ \__ \ | | | | | |  __/ |_| | | | (_) | (_| \__ \
//  \____|_|\__,_|___/___/ |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
//###########################################################################
/**--------------------------------------------------------------------------
 * Get size in bytes covered by packer/struct/reader.
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.* instance.
 * \lreturn int  size in bytes.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_GetSize( lua_State *L )
{
	struct t_pck *p  = t_pck_fld_getPackFromStack( L, 1, NULL );
	size_t        sz = t_pck_getSize( L, p );
	lua_pushinteger( L,
		(T_PCK_BTU==p->t || T_PCK_BTS==p->t || T_PCK_BOL==p->t) ? sz : sz/NB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set the default endian style of the T.Pack Constructor for fmt.
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.* instance.
 * \lreturn int  size in bytes.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_SetDefaultEndian( lua_State *L )
{
	const char *endian = luaL_checkstring( L, -1 );
	if (*endian == 'n') // native?
	{
		_default_endian = IS_LITTLE_ENDIAN;
		return 0;
	}
	luaL_argcheck( L,
		*endian == 'l' || *endian == 'b',
		-1,
		"endianness must be 'l'/'b'/'n'" );
	_default_endian =  (*endian == 'l');
	return 0;
}


/**--------------------------------------------------------------------------
 * DEBUG: Get internal reference table from a Struct/Packer.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack.* userdata instance.
 * \lreturn table  Reference table of all members.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_GetReference( lua_State *L )
{
	struct t_pck *p = t_pck_fld_getPackFromStack( L, 1, NULL );

	if (p->t > T_PCK_RAW && LUA_NOREF != p->m)
		lua_rawgeti( L, LUA_REGISTRYINDEX, p->m );
	else
		lua_pushnil( L );
	return 1;
}


//###########################################################################
//  __  __      _                         _   _               _
// |  \/  | ___| |_ __ _   _ __ ___   ___| |_| |__   ___   __| |___
// | |\/| |/ _ \ __/ _` | | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
// | |  | |  __/ || (_| | | | | | | |  __/ |_| | | | (_) | (_| \__ \
// |_|  |_|\___|\__\__,_| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
//###########################################################################

/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Pack/Field instance.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack userdata instance.
 * \lreturn string formatted string representing packer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__tostring( lua_State *L )
{
	struct t_pck_fld *pf = NULL;
	struct t_pck     *pc = t_pck_fld_getPackFromStack( L, -1, &pf );

	if (NULL == pf)
		lua_pushfstring( L, T_PCK_TYPE"." );
	else
		lua_pushfstring( L, T_PCK_FLD_TYPE"[%d](", pf->o/NB );
	t_pck_format( L, pc->t, pc->s, pc->m );
	if (NULL == pf)
		lua_pushfstring( L, "): %p", pc );
	else
		lua_pushfstring( L, "): %p", pf );
	lua_concat( L, 3 );

	return 1;
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  L     Lua state.
 * \lparam ud    T.Pack.Struct userdata instance.
 * \return int   # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__gc( lua_State *L )
{
	struct t_pck_fld *pf = NULL;
	struct t_pck     *pc = t_pck_fld_getPackFromStack( L, -1, &pf );
	if (NULL != pf)
		luaL_unref( L, LUA_REGISTRYINDEX, pf->pR );
	if (NULL == pf && pc->t > T_PCK_RAW)
		luaL_unref( L, LUA_REGISTRYINDEX, pc->m );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Field instance.
 * \param  L     Lua state.
 * \lparam  ud   T.Pack.Struct/Field instance.
 * \lreturn int  # of elements in T.Pack.Struct/Field instance.
 * \return  int  # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__len( lua_State *L )
{
	struct t_pck *pc = t_pck_fld_getPackFromStack( L, -1, NULL );

	luaL_argcheck( L, pc->t > T_PCK_RAW, 1, "Attempt to get length of atomic "T_PCK_TYPE" type" );

	lua_pushinteger( L, pc->s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call helper to read from a T.Pack.Array/Sequence/Struct instance.
 * Leaves one element on the stack.
 * \param   L       Lua state.
 * \param   struct* t_pck instance.
 * \param   char*   buffer to read from.
 * \param   size_t  running bit offset.
 * \lreturn value   value read from the buffer.
 * \return  int     # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
int
t_pck_fld__callread( lua_State *L, struct t_pck *pc, const char *b, size_t ofs )
{
	struct t_pck     *p;         ///< packer currently processing
	struct t_oht     *oht ;      ///< OrderedHashTable result if T.Struct is processed
	struct t_pck_fld *pf;        ///< packer field currently processing
	size_t            sz = 0;    ///< size of packer currently processing
	size_t            n;         ///< iterator for complex types

	//printf("%zu_", ofs);
	if (pc->t < T_PCK_ARR)       // handle atomic packer, return single value
		return t_pck_read( L, pc, b + ofs/NB, ofs%NB );
		//return t_pck_read( L, pc, b + ofs/NB );

	// for all others we need the p->m and a result table
	lua_createtable( L, pc->s, 0 );             //S:… res
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m ); //S:… res tbl/pck
	if (pc->t == T_PCK_ARR)       // handle Array; return table
	{
		p  = t_pck_check_ud( L, -1, 1 );
		sz = t_pck_getSize( L, p );

		printf( "arr: [%d:%zu]\n ", p->m, sz );

		for (n=0; n<pc->s; n++)
		{
			if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
			{
				lua_pop( L, 1 );
				p = t_pck_create_ud( L, p->t, p->s, ((p->s * n) % NB ) );
				//p = t_pck_create_ud( L, p->t, p->s,
				//((opf->ofs + ipc->s*(idx-1)) % NB ) );
			}
			t_pck_fld__callread( L, p, b, ofs + sz*n );       //S:… res typ val
			lua_rawseti( L, -3, n+1 );
		}
		lua_pop( L, 1 );
		return 1;
	}
	if (pc->t == T_PCK_SEQ)       // handle Sequence, return table
	{
		for (n=0; n<pc->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl pck
			p = t_pck_fld_getPackFromStack( L, -1, &pf );
			t_pck_fld__callread( L, p, b, ofs+sz );//S:… res tbl pck val
			sz += p->s;
			lua_rawseti( L, -4, n+1 );         //S:… res tbl pck
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
		return 1;
	}
	if (pc->t == T_PCK_STR)       // handle Struct, return oht
	{
		for (n=0; n<pc->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl key
			lua_pushvalue( L, -1 );            //S:… res tbl key key
			lua_rawget( L, -3 );               //S:… res tbl key fld
			p = t_pck_fld_getPackFromStack( L, -1, &pf );
			t_pck_fld__callread( L, p, b, ofs+sz );//S:… res tbl key pck val
			sz += p->s;
			lua_remove( L, -2 );               //S:… res tbl key val
			t_oht_addElement( L, -4 );
		}
		lua_pop( L, 1 );                      //S:… res
		oht = t_oht_create_ud( L );           //S:… res oht
		lua_insert( L, -2 );                  //S:… oht res
		oht->tR = luaL_ref( L, LUA_REGISTRYINDEX );
		return 1;
	}
	lua_pushnil( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Field/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   L     lua Virtual Machine.
 * \lparam  ud        T.Pack.Field instance.
 * \lparam  ud,string T.Buffer or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Field.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck_fld__call( lua_State *L )
{
	struct t_pck_fld  *pf  = NULL;
	struct t_pck      *pc  = t_pck_fld_getPackFromStack( L, 1, &pf );
	struct t_buf      *buf = t_buf_check_ud( L, 2, 0 );
	size_t             ofs = (NULL == pf) ? 0 : pf->o;
	char              *b;
	size_t             l;                   /// length of string  overall
	luaL_argcheck( L,  2<=lua_gettop( L ) && lua_gettop( L )<=3, 2,
		"Calling an "T_PCK_FLD_TYPE" takes 2 or 3 arguments!" );

	// are we reading/writing to from T.Buffer or Lua String
	if (NULL != buf)      // T.Buffer
	{
		l   =  buf->len;
		b   =  &(buf->b[ 0 ]);
	}
	else                  // Lua String
		b   =  (char *) luaL_checklstring( L, 2, &l );
	//printf( " %ld  %lu %zu %lu %zu \n", (NULL==pf)?-1:pf->o, ofs, o/NB, l*NB, t_pck_getSize( L, pc ) );

	luaL_argcheck( L,  l*NB+NB >= ofs + t_pck_getSize( L, pc ), 2,
		"String/Buffer must be longer than "T_PCK_TYPE" offset plus length." );

	if (2 == lua_gettop( L ))      // read from input
		return t_pck_fld__callread( L, pc, (const char *) b, ofs );
	else                           // write to input
	{
		if (pc->t < T_PCK_ARR)      // handle atomic packer, return single value
			return t_pck_write( L, pc, b + ofs/NB, ofs%NB );
		else                        // create a table ...
			return t_push_error( L, "writing of complex types is not implemented" );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_fm [] = {
	  { "__call"         , lt_pck__Call}
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_cf [] = {
	  { "getSize"        , lt_pck_GetSize }
	, { "getReference"   , lt_pck_GetReference }
	, { "setEndian"      , lt_pck_SetDefaultEndian }
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_pck_m [] = {
	  { "__call"         , lt_pck_fld__call }
	, { "__index"        , lt_pck_fld__index }
	, { "__newindex"     , lt_pck_fld__newindex }
	, { "__pairs"        , lt_pck_fld__pairs }
	, { "__gc"           , lt_pck__gc }
	, { "__len"          , lt_pck__len }
	, { "__tostring"     , lt_pck__tostring }
	, { NULL             , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the T.Pack library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_pck( lua_State *L )
{
	// T.Pack.Field instance metatable
	luaL_newmetatable( L, T_PCK_FLD_TYPE );  // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );

	// T.Pack instance metatable
	luaL_newmetatable( L, T_PCK_TYPE );      // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );
	lua_pop( L, 2 );                         // remove both metatables

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( L, t_pck_cf );
	luaL_newlib( L, t_pck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
