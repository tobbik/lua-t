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

#include "t_pck_l.h"
#include "t_buf.h"
#include "t.h"          // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif


// global default for T.Pack, can be flipped
#ifdef IS_LITTLE_ENDIAN
static int _default_endian = 1;
#else
static int _default_endian = 0;
#endif

// Function helpers
//

// ###########################################################################
//                                HELPERS adapted from Lua 5.3 Source

/** -------------------------------------------------------------------------
 * reads from string until input is not numeric any more.
 * \param   fmt    char**; format string
 * \param   dft    int   ; default value
 * \return  a      int   ; read numeric value
 *  -------------------------------------------------------------------------*/
static size_t
gn( const char **fmt, size_t dft )
{
	if (! T_PCK_ISDIGIT(** fmt))    // no number
		return dft;
	else
	{
		int a=0;
		do
		{
			a = a*10 + *((*fmt)++) - '0';
		} while (T_PCK_ISDIGIT(**fmt) &&  a <(INT_MAX/10 - 10));
		return a;
	}
}


/** -------------------------------------------------------------------------
 * Read an integer from the format parser
 * raises an error if it is larger than the maximum size for integers.
 * \param   fmt    char**; format string
 * \param   dft    int   ; default value
 * \param   max    int   ; max value allowed for int
 * \return  size   int   ; read size converted to bits
 *  -------------------------------------------------------------------------*/
static size_t
gnl( lua_State *L, const char **fmt, int dft, size_t max )
{
	size_t sz = gn( fmt, dft );
	if (sz > max || sz <= 0)
		luaL_error( L, "size (%s) out of limits [1 .. %zu]", sz, max );
	return sz * NB;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * Returns the Packer, or NULL if unsuccessful.  Leaves created packer on the
 * stack.
 * \param   *L      Lua state.
 * \param   **f     const char** format string pointer. Function moves pointer.
 * \param   *e      int* pointer. Value holds current endianess.
 * \lreturn ud      t.Pack userdata instance.
 * \return  *p      struct t_pck* pointer.
 * TODO: Deal with bit sized Packers:
 *       - Detect if we are in Bit sized type(o%8 !=0)
 *       - Detect if fmt switched back to byte style and ERROR
 *  -------------------------------------------------------------------------*/
static struct t_pck
*t_pck_parseFmt( lua_State *L, const char **f, int *e )
{
	int           opt;
	struct t_pck *p = NULL;

#define C( typ, ltl, sgn, sz ) \
   t_pck_create_ud( L, T_PCK_##typ, (sz), \
     ( ((ltl) ? T_PCK_MOD_LITTLE : 0) | ((sgn) ? T_PCK_MOD_SIGNED : 0) ) );
	while (NULL == p)
	{
		opt = *((*f)++);
		//printf("'%c'   %02X\n", opt, opt);
		switch (opt)
		{
			// Integer types
			case 'b': p = C( INT, 1==*e, 1,                         NB        ); break;
			case 'B': p = C( INT, 1==*e, 0,                         NB        ); break;
			case 'h': p = C( INT, 1==*e, 1, sizeof( short )       * NB        ); break;
			case 'H': p = C( INT, 1==*e, 0, sizeof( short )       * NB        ); break;
			case 'l': p = C( INT, 1==*e, 1, sizeof( long )        * NB        ); break;
			case 'L': p = C( INT, 1==*e, 0, sizeof( long )        * NB        ); break;
			case 'j': p = C( INT, 1==*e, 1, sizeof( lua_Integer ) * NB        ); break;
			case 'J': p = C( INT, 1==*e, 0, sizeof( lua_Integer ) * NB        ); break;
			case 'T': p = C( INT, 1==*e, 0, sizeof( size_t )      * NB        ); break;
			case 'i': p = C( INT, 1==*e, 1, gnl( L, f, sizeof( int ), MXINT ) ); break;
			case 'I': p = C( INT, 1==*e, 0, gnl( L, f, sizeof( int ), MXINT ) ); break;

			// Float types
			case 'f': p = C( FLT, 1==*e, 0, sizeof( float )       * NB        ); break;
			case 'd': p = C( FLT, 1==*e, 0, sizeof( double )      * NB        ); break;
			case 'n': p = C( FLT, 1==*e, 0, sizeof( lua_Number )  * NB        ); break;

			// String type
			case 'c': p = C( RAW,     0, 0, gnl( L, f, 1, 0x1 << 30 )         ); break;

			// Bit types
			case 'v': p = C( BOL,     0, 0, 1                                 ); break;
			case 'r': p = C( INT,     0, 1, gnl( L, f, 1, MXBIT )/NB          ); break;
			case 'R': p = C( INT,     0, 0, gnl( L, f, 1, MXBIT )/NB          ); break;

			// modifier types
			case '<': *e = 1;                                                 continue;
			case '>': *e = 0;                                                 continue;

			// allow spaces as meaningless separators
			case ' ':                                                         continue;
			// that's the end of it
			case '\0':                                                     return NULL;
			default:
				luaL_error( L, "invalid format option '%c'", opt );
				return NULL;
		}
	}
#undef C
	return p;
}


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

	if (ofs || p->s % NB)            // Bitwise parsing if not aligned to byte border
	{
		bytes  = ((p->s + ofs - 1) / NB) + 1;
		t_pck_copyBytes( out, b, bytes, 1 );
		l_shft = (MXBIT - bytes*NB + ofs);
		val    = (val << l_shft) >> (MXBIT - p->s);
	}
	else
		t_pck_copyBytes( out, b, p->s/NB, T_PCK_ISBIG( p->m ) );

	if (T_PCK_ISSIGNED( p->m ))  // 2's complement for signed
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


/* #########################################################################
 *   _                      _          _
 *  | |_ _   _ _ __   ___  | |__   ___| |_ __   ___ _ __ ___
 *  | __| | | | '_ \ / _ \ | '_ \ / _ \ | '_ \ / _ \ '__/ __|
 *  | |_| |_| | |_) |  __/ | | | |  __/ | |_) |  __/ |  \__ \
 *   \__|\__, | .__/ \___| |_| |_|\___|_| .__/ \___|_|  |___/
 *       |___/|_|                       |_|
 *  ######################################################################### */
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
		case T_PCK_BOL:
			lua_pushfstring( L, "" );
			break;
		case T_PCK_INT:
			lua_pushfstring( L, "%d%c%c", s,
			                 (T_PCK_ISSIGNED( m )) ? 's' : 'u',
			                 (T_PCK_ISLITTLE( m )) ? 'l' : 'b' );
			break;
		case T_PCK_FLT:
			lua_pushfstring( L, "%d%c", s/NB,
			                 (T_PCK_ISLITTLE( m )) ? 'l' : 'b' );
			break;
		case T_PCK_RAW:
			lua_pushfstring( L, "%d", s/NB );
			break;
		case T_PCK_ARR:
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_pushfstring( L, "[%d]", s);
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
	lua_getfield( L, -1, "t."T_PCK_IDNT );
	t_pck_format( L, t, s, m );
	lua_rawget( L, -2 );           //S: _ld t t.pck pck/nil
	if (t > T_PCK_RAW || lua_isnil( L, -1 ))  // combinator or not in cache -> create it
	{
		p    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ));
		p->t = t;
		p->s = s;
		p->m = m;

		luaL_getmetatable( L, T_PCK_TYPE );
		lua_setmetatable( L, -2 );
		if (t < T_PCK_ARR)           // register atomic types only
		{
			lua_remove( L, -2 );      // remove the nil
			t_pck_format( L, t, s, m );
			lua_pushvalue( L, -2 );   //S: _ld t t.pck pck fmt pck
			lua_rawset( L, -4 );
		}
	}
	p = t_pck_check_ud( L, -1, 1 ); //S: _ld t t.pck pck
	for (i=0; i<2; i++)
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
	if (NULL == ud && check) t_typeerror( L , pos, T_PCK_TYPE );
	return (NULL==ud) ? NULL : (struct t_pck *) ud;
}


/**--------------------------------------------------------------------------
 * Get the size of a packer of any type in bits.
 * This will mainly be needed to calculate offsets when reading.  It reads
 * recursively over Structures, Sequences and Arrays.
 * \param   L       Lua state.
 * \param   struct* t_pck.
 * \return  int     size in bits.
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
				for (n = 0; n < p->s; n++)
				{
					lua_rawgeti( L, -1, n+1 );    // get packer from table
					t_pck_fld_getPackFromStack( L, -1, NULL );
					s += t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
					lua_pop( L, 1 );
				}
				break;
			case T_PCK_STR:
				s = 0;
				for (n = 0; n < p->s; n++)
				{
					lua_rawgeti( L, -1, n+1 );
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
 * The item@pos can be a t_pck or a t_pck_fld.  The way the function depends on
 * to conditions:
 *        - item @ pos is a t_pck_fld or a t_pck
 *        - arg **pcf is NULL or points to a *t_pck_fld
 * The behaviour is as follows:
 * - if item @pos is t_pck:
 *   - returns a pointer to that userdata
 * - if item @pos is t_pck_fld
 *   - return pointer to t_pck reference in t_pck_fld
 *   - item @pos in stack will be replaced by referenced t_pck instance
 * - if item @pos is t_pck_fld and arg **pcf!=NULL
 *   - **pcf will point to t_pck_fld instance
 * \param   *L      Lua state.
 * \param    pos    int; position on Lua stack.
 * \param  **pcf    struct** pointer to t_pck_fld pointer.
 * \return  *pck    struct*  pointer to t_pck.
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
 *     - t.Pack type              : just return it
 *     - t.Pack.Field             : return referenced packer
 *     - fmt string of single item: fetch from cache or create
 *     - fmt string of mult items : let Sequence constructor handle and return result
 * \param   L      Lua state.
 * \param   pos    position on stack.
 * \param   int*   bit offset runner.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getPacker( lua_State *L, int pos )
{
	struct t_pck *p = NULL; ///< packer
	int           l = _default_endian;
	int           n = 0;    ///< counter for packers created from fmt string
	int           t = lua_gettop( L );  ///< top of stack before operations
	const char   *fmt;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	// T.Pack or T.Pack.Field at pos
	if (lua_isuserdata( L, pos ))
		p    = t_pck_fld_getPackFromStack( L, pos, NULL );
	else // format string at pos
	{
		fmt = luaL_checkstring( L, pos );
		p   = t_pck_parseFmt( L, &fmt, &l );
		while (NULL != p )
		{
			n++;
			p = t_pck_parseFmt( L, &fmt, &l );
		}
		if (n > 1)
			p =  t_pck_seq_create( L, t+1, lua_gettop( L ) );
		else
			p = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );
	}
	return p;
}


/**--------------------------------------------------------------------------
 * Read all arguments from Stack.
 * \param   L        Lua state.
 * \param   int      sp First stack index for first parameter table.
 * \param   int      ep Last  stack index for last  parameter table.
 * \lparam  mult     Sequence of tables with one key/value pair.
 * \lreturn table    Table filled according to OrderedHashTable structure.
 * \return  void.
 * --------------------------------------------------------------------------*/
static void
t_pck_readArguments( lua_State *L, int sp, int ep )
{
	size_t  i  = 0;         ///< iterator for going through the arguments
	size_t  n  = ep-sp + 1; ///< process how many arguments

	lua_createtable( L, n, n );
	while (i < n)
	{
		luaL_argcheck( L, lua_istable( L, sp ), i+1,
			"Each argument must be a table" );
		// get key/value from table
		lua_pushnil( L );                //S: sp … ep … tbl nil
		luaL_argcheck( L, lua_next( L, sp ), ep-n-1,
			"The table argument must contain a single key/value pair." );
		lua_remove( L, sp );             // remove the table now key/pck pair is on stack
		lua_pushvalue( L, -2 );          //S: sp … ep … tbl key val key
		lua_rawget( L, -4 );             //S: sp … ep … tbl key val valold?
		if (lua_isnil( L, -1 ))    // add a new value to the table
		{
			lua_pop( L, 1 );
			lua_pushvalue( L, -2 );       //S: sp … ep … tbl key val key
			lua_rawseti( L, -4, lua_rawlen( L, -4 )+1 );
			lua_rawset( L, -3 );
		}
		else
			luaL_error( L, "No duplicates for Pack keys allowed" );

		i++;
	}
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
	lua_remove( L, 1 );               // remove the t.Pack Class table
	if (lua_istable( L, 1 ))          // Oht style constructor -> struct
	{
		t_pck_readArguments( L, 1, lua_gettop( L ) );
		t_pck_str_create( L );         //S: pck tbl
	}
	else
	{
		if (1==lua_gettop( L ))
			t_pck_getPacker( L, 1 );    // single packer
		else
			if (lua_isinteger( L, 2 ))
				t_pck_arr_create( L );   // if second is number it must be array
			else
				t_pck_seq_create( L, 1, lua_gettop( L ) );
	}
	return 1;
}


/* ############################################################################
 *    ____ _                                _   _               _
 *   / ___| | __ _ ___ ___   _ __ ___   ___| |_| |__   ___   __| |___
 *  | |   | |/ _` / __/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 *  | |___| | (_| \__ \__ \ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 *   \____|_|\__,_|___/___/ |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
 * ######################################################################### */
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
	lua_pushinteger( L, sz/NB );   // size in bytes
	lua_pushinteger( L, sz );      // size in bits
	return 2;
}


/**--------------------------------------------------------------------------
 * Get offset of a Pack.Field.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack.Field userdata instance.
 * \lreturn bytes  offset from beginning in bytes.
 * \lreturn bits   offset from beginning in bits.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_fld_GetOffset( lua_State *L )
{
	struct t_pck_fld *pf = NULL;
	t_pck_fld_getPackFromStack( L, 1, &pf );
	luaL_argcheck( L, NULL!=pf, 1, "Expected `"T_PCK_FLD_TYPE"`." );
	lua_pushinteger( L, pf->o/NB );    // offset in Bytes
	lua_pushinteger( L, pf->o );       // offset in Bits
	return 2;
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
	char endian = *(luaL_checkstring( L, 1 ));
	if ('n' == endian) // native?
		endian = (IS_LITTLE_ENDIAN) ? 'l' : 'b';
	luaL_argcheck( L, endian == 'l' || endian == 'b', 1,
		"endianness must be 'l'/'b'/'n'" );
	_default_endian = (endian == 'l');
	return 0;
}


/**--------------------------------------------------------------------------
 * Get the specific subType of a Packer/Field.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack/T.Pack.Field userdata instance.
 * \lreturn string Name of pack type.
 * \lreturn string Name of pack sub-type.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_Type( lua_State *L )
{
	struct t_pck *p = t_pck_fld_getPackFromStack( L, 1, NULL );
	lua_pushfstring( L, "%s", t_pck_t_lst[ p->t ] );
	t_pck_format( L, p->t, p->s, p->m );
	return 2;
}


/* ###########################################################################
//  __  __      _                         _   _               _
// |  \/  | ___| |_ __ _   _ __ ___   ___| |_| |__   ___   __| |___
// | |\/| |/ _ \ __/ _` | | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
// | |  | |  __/ || (_| | | | | | | |  __/ |_| | | | (_) | (_| \__ \
// |_|  |_|\___|\__\__,_| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
//#########################################################################  */

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
		lua_pushfstring( L,  ": %p", pc );
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
	{
		luaL_unref( L, LUA_REGISTRYINDEX, pf->pR );
		if (LUA_REFNIL != pf->bR)
			luaL_unref( L, LUA_REGISTRYINDEX, pf->bR );
	}
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
 * \return  offset  after read.
 * -------------------------------------------------------------------------*/
size_t
t_pck_fld__callread( lua_State *L, struct t_pck *pc, const char *b, size_t ofs )
{
	struct t_pck     *p;         ///< packer currently processing
	struct t_pck_fld *pf;        ///< packer field currently processing
	size_t            n;         ///< iterator for complex types

	if (pc->t < T_PCK_ARR)       // handle atomic packer, return single value
	{
		t_pck_read( L, b + ofs/NB, pc, ofs%NB );
		return ofs + pc->s;
	}

	// for all others we need the p->m and a result table
	lua_createtable( L, pc->s, 0 );             //S:… res
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m ); //S:… res tbl/pck
	if (pc->t == T_PCK_ARR)       // handle Array; return table
	{
		p  = t_pck_check_ud( L, -1, 1 );
		for (n=0; n<pc->s; n++)
		{
			ofs = t_pck_fld__callread( L, p, b, ofs );       // S:… res typ val
			lua_rawseti( L, -3, n+1 );
		}
		lua_pop( L, 1 );
		return ofs;
	}
	if (pc->t == T_PCK_SEQ)       // handle Sequence, return table
	{
		for (n=0; n<pc->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl pck
			p   = t_pck_fld_getPackFromStack( L, -1, &pf );
			ofs = t_pck_fld__callread( L, p, b, ofs );//S:… res tbl pck val
			lua_rawseti( L, -4, n+1 );         //S:… res tbl pck
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
		return ofs;
	}
	if (pc->t == T_PCK_STR)       // handle Struct, return oht
	{
		for (n=0; n<pc->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl key
			lua_pushvalue( L, -1 );            //S:… res tbl key key
			lua_rawget( L, -3 );               //S:… res tbl key fld
			p   = t_pck_fld_getPackFromStack( L, -1, &pf );
			ofs = t_pck_fld__callread( L, p, b, ofs );//S:… res tbl key pck val
			lua_remove( L, -2 );               //S:… res tbl key val
			lua_pushvalue( L, -2 );            //S:… res tbl key val key
			lua_rawseti( L, -5, lua_rawlen( L, -5 )+1 );
			lua_rawset( L, -4 );
		}
		lua_pop( L, 1 );                      //S:… res

		lua_newtable( L );                    //S:… res tbl
		luaL_getmetatable( L, "T.ProxyTableIndex" ); //S:… res tbl prx
		lua_rotate( L, -3, -1 );              //S:… tbl prx res
		lua_rawset( L, -3 );;                 //S:… tbl
		luaL_getmetatable( L, "T.OrderedHashTable" );
		lua_setmetatable( L, -2 );

		return ofs;
	}
	lua_pushnil( L );
	return ofs;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Field/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   L         Lua state.
 * \lparam  ud        T.Pack.Field instance.
 * \lparam  ud,string T.Buffer, t.Buffer.Segment or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Field.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck_fld__call( lua_State *L )
{
	struct t_pck_fld  *pf  = NULL;
	struct t_pck      *pc  = t_pck_fld_getPackFromStack( L, 1, &pf );
	size_t             ofs = (NULL == pf) ? 0 : pf->o;
	char              *b;
	size_t             l;                   /// length of string  overall
	int                canwrite;            /// false if string is passed

	luaL_argcheck( L,  2<=lua_gettop( L ) && lua_gettop( L )<=3, 2,
		"Calling an "T_PCK_FLD_TYPE" takes 2 or 3 arguments!" );

	b = t_buf_checklstring( L, 2, &l, &canwrite );
	//printf( " %ld  %lu %zu %lu %zu \n", (NULL==pf)?-1:pf->o, ofs, o/NB, l*NB, t_pck_getSize( L, pc ) );

	luaL_argcheck( L, (l*NB)+NB >= ofs + t_pck_getSize( L, pc ), 2,
		"String/Buffer must be longer than "T_PCK_TYPE" offset plus length." );

	if (2 == lua_gettop( L ))      // read from input
	{
		t_pck_fld__callread( L, pc, (const char *) b, ofs );
		return 1;
	}
	else                           // write to input
	{
		luaL_argcheck( L, (canwrite), 2, "Can't write value to string type" );
		if (pc->t < T_PCK_ARR)      // handle atomic packer, return single value
			return t_pck_write( L, b + ofs/NB, pc, ofs%NB );
		else                        // create a table ...
			return luaL_error( L, "writing of complex types is not implemented" );
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
	  { "size"           , lt_pck_GetSize }
	, { "defaultEndian"  , lt_pck_SetDefaultEndian }
	, { "type"           , lt_pck_Type }
	, { "offset"         , lt_pck_fld_GetOffset }
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
	lua_pushinteger( L, sizeof( int ) );
	lua_setfield( L, -2, "intsize" );
	lua_pushinteger( L, NB );
	lua_setfield( L, -2, "charbits" );
	lua_pushinteger( L, MXINT );
	lua_setfield( L, -2, "numsize" );
	luaL_newlib( L, t_pck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
