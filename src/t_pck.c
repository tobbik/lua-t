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


#include <string.h>               // memset

#include "t.h"
#include "t_buf.h"

// ========== Buffer accessor Helpers

/* number of bits in a character */
#define NB                 CHAR_BIT

/* mask for one character (NB 1's) */
#define MC                 ((1 << NB) - 1)

/* size of a lua_Integer */
#define MXINT              ((int)sizeof(lua_Integer))

// Maximum bits that can be read or written
#define MXBIT              MXINT * NB

// Macro helpers
#define BIT_GET(b,n)       ( ((b) >> (NB-(n))) & 0x01 )
#define BIT_SET(b,n,v)     \
	( (1==v)              ? \
	 ((b) | (  (0x01) << (NB-(n))))    : \
	 ((b) & (~((0x01) << (NB-(n))))) )

// global default for T.Pack, can be flipped
#ifdef IS_LITTLE_ENDIAN
static int _default_endian = 1;
#else
static int _default_endian = 0;
#endif

// Declaration because of circular dependency
static struct t_pck *t_pck_mksequence( lua_State *L, int sp, int ep, size_t *bo );


// Function helpers
/**--------------------------------------------------------------------------
 * Copy byte by byte from one string to another. Honours endianess.
 * \param   dst       pointer to char array to write to.
 * \param   src       pointer to char array to read from.
 * \param   sz        how many bytes to copy.
 * \param   islittle  treat input as little endian?
 * --------------------------------------------------------------------------*/
static void
t_pck_cbytes( unsigned char * dst, const unsigned char * src, size_t sz, int islittle )
{
	if (IS_LITTLE_ENDIAN == islittle)
		while (sz-- != 0)
			*(dst++) = *(src+sz);
	else
	{
		src = src+sz-1;
		while (sz-- != 0)
			*(dst++) = *(src-sz);
	}
}


/**--------------------------------------------------------------------------
 * Write an integer of y bits from to char buffer with offset ofs.
 * \param   val  the val gets written to.
 * \param   len  size in bits (1-64).
 * \param   ofs  offset   in bits (0-7).
 * \param  *buf  char buffer already on proper position
 * --------------------------------------------------------------------------*/
static void
t_pck_wbits( lua_Unsigned val, size_t len, size_t ofs, unsigned char * buf )
{
	lua_Unsigned   set = 0;                           ///< value for the read access
	lua_Unsigned   msk = 0;                           ///< mask
	/// how many bit are in all the bytes needed for the conversion
	size_t     abyt = ((len+ofs-1)/NB) + 1;
	size_t     abit = abyt* 8;

	msk = (-1 << (MXBIT-len)) >> (MXBIT-abit+ofs);
#ifdef IS_LITTLE_ENDIAN
	t_pck_cbytes( (unsigned char *) &set, buf, abyt, 1);
#else
	t_pck_cbytes( 
	   (unsigned char *) &set + sizeof( lua_Unsigned) - abyt,
	   buf,
		abyt,
		0 );
#endif
	// isolate the value and merge with pre-existing bits
	set = (val << (abit-ofs-len)) | (set & ~msk);
#ifdef IS_LITTLE_ENDIAN
	t_pck_cbytes( buf, (unsigned char *) &set, abyt, 1);
#else
	t_pck_cbytes( 
	   buf,
	   (unsigned char *) &set + sizeof( lua_Unsigned) - abyt,
		abyt,
		0 );
#endif

#if PRINT_DEBUGS == 1
	printf("Read: %016llX       \nLft:  %016lX       %d \nMsk:  %016lX       %ld\n"
	       "Nmsk: %016llX       \nval:  %016llX         \n"
	       "Sval: %016llX    %ld\nRslt: %016llX         \n",
			val,
			 -1 << (MXBIT-len), (MXBIT-len),  /// Mask after left shift
			(-1 << (MXBIT-len)) >> (MXBIT-abit+ofs), (MXBIT-abit+ofs),
			set & ~msk,
			val,
			 val << (abit-ofs-len),  abit-ofs-len,
			(val << (abit-ofs-len)) | (set & ~msk)
			);
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC t_pck API ========================
// Reader and writer for packer data
/**--------------------------------------------------------------------------
 * reads a value from the packer and pushes it onto the Lua stack.
 * \param   L lua Virtual Machine.
 * \param   struct t_pack.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------- */
int
t_pck_read( lua_State *L, struct t_pck *p, const unsigned char *b )
{
	lua_Unsigned           msk=0, val=0;
	volatile union Ftypes  u;
	switch( p->t )
	{
		case T_PCK_INT:
#ifdef IS_LITTLE_ENDIAN
			t_pck_cbytes( (unsigned char *) &val, b, p->s, ! p->m );
#else
			t_pck_cbytes( 
			   (unsigned char *) &val + sizeof( lua_Unsigned) - p->s,
			   b,
				p->s,
				p->m );
#endif
			msk = (lua_Unsigned) 1  << (p->s*NB - 1);
			lua_pushinteger( L, (lua_Integer) ((val ^ msk) - msk) );
			break;
		case T_PCK_UNT:
			if (1 == p->s)
				lua_pushinteger( L, (lua_Integer) *b );
			else
			{
#ifdef IS_LITTLE_ENDIAN
				t_pck_cbytes( (unsigned char *) &val, b, p->s, ! p->m );
#else
				t_pck_cbytes( 
				   (unsigned char *) &val + sizeof( lua_Unsigned) - p->s,
				   b,
					p->s,
					p->m );
#endif
				lua_pushinteger( L, (lua_Integer) val );
			}
			break;
		case T_PCK_BOL:
			lua_pushboolean( L, BIT_GET( *b, p->m ) );
			break;
		case T_PCK_BTS:
			if (p->s == 1)
				lua_pushinteger( L, BIT_GET( *b, p->m ) );
			else
			{
				msk = (lua_Unsigned) 1  << (p->s - 1);
				// copy as many bytes as needed
#ifdef IS_LITTLE_ENDIAN
				t_pck_cbytes( (unsigned char *) &val, b, (p->s+p->m-1)/8 + 1, 1 );
#else
				t_pck_cbytes( 
				   (unsigned char *) &val + sizeof( lua_Unsigned) - (p->s+p->m-1)/8 + 1,
				   b,
					(p->s+p->m-1)/8 + 1,
					0 );
#endif
				val = (val << (MXBIT- ((p->s/NB+1)*NB) + p->m ) ) >> (MXBIT - p->s);
				lua_pushinteger( L, (lua_Integer) ((val ^ msk) - msk) );
			}
			break;
		case T_PCK_BTU:
			if (p->s == 1)
				lua_pushinteger( L, BIT_GET( *b, p->m ) );
			else
			{
				// copy as many bytes as needed
#ifdef IS_LITTLE_ENDIAN
				t_pck_cbytes( (unsigned char *) &val, b, (p->s+p->m-1)/8 + 1, 1 );
#else
				t_pck_cbytes( 
				   (unsigned char *) &val + sizeof( lua_Unsigned) - (p->s+p->m-1)/8 + 1,
				   b,
					(p->s+p->m-1)/8 + 1,
					0 );
#endif
				val = (val << (MXBIT- ((p->s/NB+1)*NB) + p->m ) ) >> (MXBIT - p->s);
				lua_pushinteger( L, (lua_Integer) val );
			}
			break;
		case T_PCK_FLT:
			t_pck_cbytes( (unsigned char*) &(u), b, p->s, 0 );
			if      (sizeof( u.f ) == p->s) lua_pushnumber( L, (lua_Number) u.f );
			else if (sizeof( u.d ) == p->s) lua_pushnumber( L, (lua_Number) u.d );
			else                            lua_pushnumber( L, u.n );
			break;
		case T_PCK_RAW:
			lua_pushlstring( L, (const char*) b, p->s );
			break;
		default:
			return t_push_error( L, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Sets a value from stack to a char buffer according to paccker format.
 * \param  L lua Virtual Machine.
 * \param  struct t_pack.
 * \param  unsigned char* char buffer to write to.
 * \lparam Lua value.
 *
 * return integer return code -0==success; !=0 means errors pushed to Lua stack
 *  -------------------------------------------------------------------------*/
int
t_pck_write( lua_State *L, struct t_pck *p, unsigned char *b )
{
	lua_Integer            intVal;
	lua_Unsigned           msk=0, val=0;
	volatile union Ftypes  u;
	const char            *strVal;
	size_t                 sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case T_PCK_INT:
			intVal = luaL_checkinteger( L, -1 );
			val    = (lua_Unsigned) intVal;
			if (0>intVal && p->s != MXINT)
			{
				msk = (lua_Unsigned) 1  << (p->s*NB - 1);
				val = ((val ^ msk) - msk);
			}
			luaL_argcheck( L,  0 == (val >> (p->s*NB)) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (1==p->s)
				*b = (char) val;
			else
#ifdef IS_LITTLE_ENDIAN
				t_pck_cbytes( b, (unsigned char *) &val, p->s, ! p->m );
#else
				t_pck_cbytes(
				   b,
				   (unsigned char *) &val + sizeof( lua_Unsigned) - p->s ,
				   p->s,
				   p->m );
#endif
				//t_pck_wbytes( val, p->s, p->m, b );
			break;
		case T_PCK_UNT:
			intVal = luaL_checkinteger( L, -1 );
			val    = (lua_Unsigned) intVal;
			luaL_argcheck( L,  0 == (val >> (p->s*NB)) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (1==p->s)
				*b = (char) val;
			else
#ifdef IS_LITTLE_ENDIAN
				t_pck_cbytes( b, (unsigned char *) &val, p->s, ! p->m );
#else
				t_pck_cbytes(
				   b,
				   (unsigned char *) &val + sizeof( lua_Unsigned) - p->s ,
				   p->s,
				   p->m );
#endif
			break;
		case T_PCK_BOL:
			luaL_argcheck( L,  lua_isboolean( L, -1 ) , -1,
			   "value to pack must be boolean type" );
			*b = BIT_SET( *b, p->m, lua_toboolean( L, -1 ) );
			break;
		case T_PCK_BTS:
			//TODO: proper boundary checking!
			intVal = luaL_checkinteger( L, -1 );
			val    = (lua_Unsigned) intVal << (MXBIT- p->s) >> (MXBIT-p->s);
			luaL_argcheck( L,  0 == (val >> p->s) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (p->s == 1)
				*b = BIT_SET( *b, p->m, lua_toboolean( L, -1 ) );
			else
				t_pck_wbits( val, p->s, p->m, b );
			break;
		case T_PCK_BTU:
			intVal = luaL_checkinteger( L, -1 );
			luaL_argcheck( L,  0 == (intVal >> p->s) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (p->s == 1)
				*b = BIT_SET( *b, p->m, lua_toboolean( L, -1 ) );
			else
				t_pck_wbits( (lua_Unsigned) intVal, p->s, p->m, b );
			break;
		case T_PCK_FLT:
			if      (sizeof( u.f ) == p->s) u.f = (float)  luaL_checknumber( L, -1 );
			else if (sizeof( u.d ) == p->s) u.d = (double) luaL_checknumber( L, -1 );
			else                            u.n = luaL_checknumber( L, -1 );
			t_pck_cbytes( b, (unsigned char*) &(u), p->s, 0 );
			break;
		case T_PCK_RAW:
			strVal = luaL_checklstring( L, -1, &sL );
			luaL_argcheck( L,  p->s >= sL, -1, "String is to big for the field" );
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
 * \param   L     The lua state.
 * \param   t_pack    the packer instance struct.
 * \lreturn  leaves two strings on the Lua Stack.
 * --------------------------------------------------------------------------*/
void
t_pck_format( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	lua_pushfstring( L, "%s", t_pck_t_lst[ t ] );
	switch( t )
	{
		case T_PCK_INT:
		case T_PCK_UNT:
			lua_pushfstring( L, "%d%c", s, (1==m) ? 'L' : 'B' );
			break;
		case T_PCK_FLT:
			lua_pushfstring( L, "%d", s );
			break;
		case T_PCK_BOL:
			lua_pushfstring( L, "%d", m+1 );
			break;
		case T_PCK_BTS:
		case T_PCK_BTU:
			lua_pushfstring( L, "%d:%d", s, m+1 );
			break;
		case T_PCK_RAW:
			lua_pushfstring( L, "%d", s );
			break;
		case T_PCK_ARR:
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_pushfstring( L, "[%d](%d)", s, m );
			break;
		default:
			lua_pushfstring( L, "UNKNOWN");
	}
}


/**--------------------------------------------------------------------------
 * See if requested type exists in T.Pack. otherwise create and register.
 * the format for a particular definition will never change. Hence, no need to
 * create them over and over again.  This approach saves memory.
 * \param     L  lua state.
 * \param     enum   t_pck_t.
 * \param     size   number of elements.
 * \param     mod parameter.
 * \return    struct t_pck* pointer. create a t_pack and push to LuaStack.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_create_ud( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	struct t_pck  __attribute__ ((unused)) *p;
	int                                     i;

	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( L, -1, "t" );
	lua_getfield( L, -1, T_PCK_NAME );
	t_pck_format( L, t, s, m ); lua_concat( L, 2 );
	lua_rawget( L, -2 );
	if (lua_isnil( L, -1 ))    // havent found in cache -> create it
	{
		lua_pop( L, 1 );        // pop the nil
		p = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ));
		p->t = t;
		p->s = s;
		p->m = m;

		luaL_getmetatable( L, T_PCK_TYPE );
		lua_setmetatable( L, -2 );
		t_pck_format( L, t, s, m ); lua_concat( L, 2 );
		lua_pushvalue( L, -2 );
		lua_rawset( L, -4 );
	}
	p = t_pck_check_ud( L, -1, 1 );
	for (i=0; i<3; i++)
		lua_remove( L, -2 );

	return p;
}


/**--------------------------------------------------------------------------
 * Check if value on stack is T.Pack OR * T.Pack.Struct/Sequence/Array
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> error if fail
 * \lparam  userdata T.Pack/Struct on the stack.
 * \return  t_pck pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_PCK_TYPE );
	if (NULL != ud)
		return (struct t_pck *) ud;
	if (check)
		luaL_argcheck( L, ud != NULL, pos, "`"T_PCK_TYPE"` expected" );
	return NULL;
}


/**--------------------------------------------------------------------------
 * Get the size of a packer of any type in bytes.
 * This will mainly be needed to calculate offsets when reading.
 * \param   L    The lua state.
 * \param   struct t_pck.
 * \param   int    bits - boolean if bit resolution is needed.
 * \return  size in bytes.
 * TODO: return 0 no matter if even one item is of unknown length.
 * --------------------------------------------------------------------------*/
static size_t 
t_pck_getsize( lua_State *L,  struct t_pck *p, int bits )
{
	size_t        s = 0;
	size_t        n;       ///< iterator over accumulated
	switch (p->t)
	{
		case T_PCK_INT:
		case T_PCK_UNT:
		case T_PCK_FLT:
		case T_PCK_RAW:
			return ((bits)
					? 8*p->s
					: p->s);
			break;
		case T_PCK_BOL:
			return 1;
			break;
		case T_PCK_BTS:
		case T_PCK_BTU:
			return ((bits)
					? p->s
					: ((p->s + p->m - 1)/NB) + 1);
			break;
		case T_PCK_ARR:
			lua_rawgeti( L, LUA_REGISTRYINDEX, p->m ); // get packer
			s = p->s * t_pck_getsize( L, t_pck_check_ud( L, -1, 1 ), 1 );
			lua_pop( L, 1 );
			return ((bits)
					? s
					: s/NB);
			break;
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_rawgeti( L, LUA_REGISTRYINDEX, p->m ); // get table
			for (n = 1; n <= p->s; n++)
			{
				//t_stackDump(L);
				lua_rawgeti( L, -1, n );
				s += t_pck_getsize( L, t_pck_check_ud( L, -1, 1 ), 1 );
				lua_pop( L, 1 );
			}
			lua_pop( L, 1 );
			return ((bits)
					? s
					: s/NB);
			break;
		default:
			return 0;
	}
}


// ###########################################################################
//                                HELPERS adapted from Lua 5.3 Source
/** -------------------------------------------------------------------------
 * See if int represents a character which is a digit.
 * \param     int c
 * \return    boolean 0:flase - 1:true
 *  -------------------------------------------------------------------------*/
static int
is_digit( int c ) { return '0' <= c && c<='9'; }


/** -------------------------------------------------------------------------
 * reads from string until input is not numeric any more.
 * \param     char** format string
 * \param     int    default value
 * \return    int    read numeric value
 *  -------------------------------------------------------------------------*/
static int
gn( const char **fmt, int df )
{
	if (! is_digit(** fmt))    // no number
		return df;
	else
	{
		int a=0;
		do
		{
			a = a*10+ *((*fmt)++) - '0';
		} while (is_digit(**fmt) &&  a <(INT_MAX/10 - 10));
		return a;
	}
}


/** -------------------------------------------------------------------------
 * Read an integer from the format parser
 * raises an error if it is larger than the maximum size for integers.
 * \param  char* format string
 * \param  int   default value if no number is in the format string
 * \param  int   max value allowed for int
 * \return the converted integer value
 *  -------------------------------------------------------------------------*/
static int
gnl( lua_State *L, const char **fmt, int df, int max )
{
	int sz = gn( fmt, df );
	if (sz > max || sz <= 0)
		luaL_error( L, "size (%d) out of limits [1,%d]", sz, max );
	return sz;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * Returns the Packer, or NULL if unsuccessful.
 * \param     L  lua state.
 * \param     char*  format string pointer. moved by this function.
 * \param     int*   e pointer to current endianess.
 * \param     int*   bo pointer to current bit offset within byte.
 * \return    struct t_pck* pointer.
 * TODO: Deal with bit sized Packers:
 *       - Detect if we are in Bit sized type(o%8 !=0)
 *       - Detect if fmt switched back to byte style and ERROR
 *  -------------------------------------------------------------------------*/
struct t_pck
*t_pck_getoption( lua_State *L, const char **f, int *e, size_t *bo )
{
	int           opt;
	int           m;
	size_t        s;
	enum t_pck_t  t;
	struct t_pck *p   = NULL;

	while (NULL == p)
	{
		opt = *((*f)++);
		//printf("'%c'   %02X\n", opt, opt);
		switch (opt)
		{
			// Integer types
			case 'b': t = T_PCK_INT;   m = (1==*e);   s = 1;                                 break;
			case 'B': t = T_PCK_UNT;   m = (1==*e);   s = 1;                                 break;
			case 'h': t = T_PCK_INT;   m = (1==*e);   s = sizeof( short );                   break;
			case 'H': t = T_PCK_UNT;   m = (1==*e);   s = sizeof( short );                   break;
			case 'l': t = T_PCK_INT;   m = (1==*e);   s = sizeof( long );                    break;
			case 'L': t = T_PCK_UNT;   m = (1==*e);   s = sizeof( long );                    break;
			case 'j': t = T_PCK_INT;   m = (1==*e);   s = sizeof( lua_Integer );             break;
			case 'J': t = T_PCK_UNT;   m = (1==*e);   s = sizeof( lua_Integer );             break;
			case 'T': t = T_PCK_INT;   m = (1==*e);   s = sizeof( size_t );                  break;
			case 'i': t = T_PCK_INT;   m = (1==*e);   s = gnl( L, f, sizeof( int ), MXINT ); break;
			case 'I': t = T_PCK_UNT;   m = (1==*e);   s = gnl( L, f, sizeof( int ), MXINT ); break;

			// Float types
			case 'f': t = T_PCK_FLT;   m = (1==*e);   s = sizeof( float );                   break;
			case 'd': t = T_PCK_FLT;   m = (1==*e);   s = sizeof( double );                  break;
			case 'n': t = T_PCK_FLT;   m = (1==*e);   s = sizeof( lua_Number );              break;

			// String type
			case 'c': t = T_PCK_RAW;   m = 0;         s = gnl( L, f, 1, 0x1 << NB );         break;

			// Bit types
			case 'v':
				t = T_PCK_BOL;
				m = gnl(L, f, 1+(*bo%NB), NB) - 1;
				s = 1;
				break;
			case 'r':
				t = T_PCK_BTS;
				m = *bo % NB;
				s = gnl(L, f, 1, MXBIT );
				break;
			case 'R':
				t = T_PCK_BTU;
				m = *bo % NB;
				s = gnl(L, f, 1, MXBIT );
				break;

			// modifier types
			case '<': *e = 1; continue;                                                      break;
			case '>': *e = 0; continue;                                                      break;
			case '\0': return NULL;                                                          break;
			default:
				luaL_error( L, "invalid format option '%c'", opt );
				return NULL;
		}
		// TODO: check if 0==offset%8 if byte type, else error
		p    = t_pck_create_ud( L, t, s, m );
		// forward the Bit offset
		*bo += ((T_PCK_BTU==t || T_PCK_BTS==t || T_PCK_BOL == t) ? s : s*NB);
	}
	return p;
}


/**--------------------------------------------------------------------------
 * Get T.Pack from a stack element.
 * Return Reader Pointer if requested.
 * \param  L lua Virtual Machine.
 * \param  position on Lua stack.
 * \param  pointer to reader pointer.
 * \return pointer to t_pck struct*.
 * --------------------------------------------------------------------------*/
static inline struct t_pck
*t_pck_getpckreader( lua_State * L, int pos, struct t_pcr **prp )
{
	void         *ud = luaL_testudata( L, pos, T_PCK_TYPE".Reader" );
	struct t_pcr *pr = (NULL == ud) ? NULL : (struct t_pcr *) ud;
	struct t_pck *pc;
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	if (NULL == pr)
		return t_pck_check_ud( L, pos, 1 );
	else
	{
		if (NULL != prp)
			//*prp = *(&pr);
			*prp = pr;
		lua_rawgeti( L, LUA_REGISTRYINDEX, pr->r );
		pc = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );
		return pc;
	}
}


/**--------------------------------------------------------------------------
 * Decides if the element on pos is a packer kind of type.
 * It decides between the following options:
 *     - T.Pack type              : just return it
 *     - T.Pack.Reader            : return reference packer
 *     - fmt string of single item: fetch from cache or create
 *     - fmt string of mult items : let Sequence constructor handle and return result
 * \param   L  The lua state.
 * \param   pos    position on stack.
 * \param   atom   boolean atomic packers only.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getpck( lua_State *L, int pos, size_t *bo )
{
	struct t_pck *p = NULL; ///< packer
	int           l = _default_endian;
	int           n = 0;  ///< counter for packers created from fmt string
	int           t = lua_gettop( L );  ///< top of stack before operations
	const char   *fmt;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	// if it is a T.Pack or T.Pack.Reader
	if (lua_isuserdata( L, pos ))
	{
		p    = t_pck_getpckreader( L, pos, NULL );
		// This fixes Bitwise offsets
		if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
		{
			p = t_pck_create_ud( L, p->t, p->s, *bo%NB );
			lua_replace( L, pos );
		}
		*bo += t_pck_getsize( L, p, 1 );
	}
	else // if it is a format string
	{
		fmt = luaL_checkstring( L, pos );
		p   = t_pck_getoption( L, &fmt, &l, bo );
		while (NULL != p )
		{
			n++;
			p   = t_pck_getoption( L, &fmt, &l, bo );
		}
		// TODO: actually create the packers and calculate positions
		if (1 < n)
		{
			p =  t_pck_mksequence( L, t+1, lua_gettop( L ), bo );
		}
		else
			p = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );
	}
	return p;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Array Object and put it onto the stack.
 * \param   L  The lua state.
 * \lparam  type identifier  T.Pack, T.Pack.Struct, T.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mkarray( lua_State *L )
{
	size_t            bo = 0;
	struct t_pck  __attribute__ ((unused))   *p  = t_pck_getpck( L, -2, &bo );  ///< packer
	struct t_pck     *ap;     ///< array userdata to be created

	ap    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	ap->t = T_PCK_ARR;
	ap->s = luaL_checkinteger( L, -2 );      // how many elements in the array

	lua_pushvalue( L, -3 );  // Stack: Pack,n,Array,Pack
	ap->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register packer table

	luaL_getmetatable( L, T_PCK_TYPE );
	lua_setmetatable( L, -2 ) ;

	return ap;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Sequence Object and put it onto the stack.
 * \param   L  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mksequence( lua_State *L, int sp, int ep, size_t *bo )
{
	size_t        n=1;    ///< iterator for going through the arguments
	size_t        o=0;    ///< byte offset within the sequence
	struct t_pck *p;      ///< temporary packer/struct for iteration
	struct t_pck *sq;     ///< the userdata this constructor creates

	sq     = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	sq->t  = T_PCK_SEQ;
	sq->s  = (ep-sp)+1;

	// create and populate index table
	lua_newtable( L );                  // Stack: fmt,Seq,idx
	while (n <= sq->s)
	{
		p = t_pck_getpck( L, sp, bo );
		lua_pushvalue( L, sp );          // Stack: fmt,Seq,idx,Pack
		lua_pushinteger( L, o/8 );       // Stack: fmt,Seq,idx,Pack,ofs
		lua_rawseti( L, -3, n + sq->s ); // Stack: fmt,Seq,idx,Pack     idx[n+i] = offset
		lua_rawseti( L, -2, n );         // Stack: fmt,Seq,idx,         idx[i]   = Pack
		o += t_pck_getsize( L, p, 1 );
		n++;
		lua_remove( L, sp );
	}
	sq->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register index  table

	luaL_getmetatable( L, T_PCK_TYPE ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( L, -2 ) ;

	return sq;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Struct Object and put it onto the stack.
 * \param   L  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \lparam  ... multiple of type  table { name = T.Pack}.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mkstruct( lua_State *L, int sp, int ep )
{
	size_t        n  = 1;  ///< iterator for going through the arguments
	size_t        o  = 0;  ///< byte offset within the sequence
	size_t        bo = 0;  ///< bit  offset within the sequence
	struct t_pck *p;       ///< temporary packer/struct for iteration
	struct t_pck *st;      ///< the userdata this constructor creates

	st     = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	st->t  = T_PCK_STR;
	st->s  = (ep-sp) + 1;

	// create and populate index table
	lua_newtable( L );                  // S:...,Struct,idx
	while (n <= st->s)
	{
		luaL_argcheck( L, lua_istable( L, sp ), n,
			"Arguments must be tables with single key/"T_PCK_TYPE" pair" );
		// Stack gymnastic:
		lua_pushnil( L );
		if (! lua_next( L, sp ))         // S:...,Struct,idx,name,Pack
			luaL_error( L, "The table argument must contain one key/value pair." );
		// check if name is already used!
		lua_pushvalue( L, -2 );          // S:...,Struct,idx,name,Pack,name
		lua_rawget( L, -4 );             // S:...,Struct,idx,name,Pack,nil?
		if (! lua_isnoneornil( L, -1 ))
			luaL_error( L, "All elements in "T_PCK_TYPE".Struct must have unique key." );
		lua_pop( L, 1 );                 // S:...,Struct,idx,name,Pack
		p = t_pck_getpck( L, -1, &bo );  // allow T.Pack or T.Pack.Struct
		// populate idx table
		lua_pushinteger( L, o/8 );       // S:...,Struct,idx,name,Pack,ofs
		lua_rawseti( L, -4, n + st->s ); // S:...,Struct,idx,name,Pack        idx[n+i] = offset
		lua_rawseti( L, -3, n );         // S:...,Struct,idx,name             idx[i] = Pack
		lua_pushvalue( L, -1 );          // S:...,Struct,idx,name,name
		lua_rawseti( L, -3, st->s*2+n ); // S:...,Struct,idx,name             idx[2n+i] = name
		lua_pushinteger( L, n);          // S:...,Struct,idx,name,i
		lua_rawset( L, -3 );             // S:...,Struct,idx                  idx[name] = i
		o += t_pck_getsize( L, p, 1 );
		n++;
		lua_remove( L, sp );
	}

	st->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register index  table

	luaL_getmetatable( L, T_PCK_TYPE ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( L, -2 ) ;

	return st;
}

//###########################################################################
//   ____                _                   _
//  / ___|___  _ __  ___| |_ _ __ _   _  ___| |_ ___  _ __
// | |   / _ \| '_ \/ __| __| '__| | | |/ __| __/ _ \| '__|
// | |__| (_) | | | \__ \ |_| |  | |_| | (__| || (_) | |
//  \____\___/|_| |_|___/\__|_|   \__,_|\___|\__\___/|_|
//###########################################################################
/** -------------------------------------------------------------------------
 * Creates a packerfrom the function call.
 * \param     L  lua state.
 * \lparam    fmt    string.
 *      or
 * \lparam    T.Pack elements.
 *      or
 * \lparam    {name=T.Pack}, ...  name value pairs.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pck_New( lua_State *L )
{
	struct t_pck  __attribute__ ((unused)) *p;
	size_t                                  bo = 0;

	// Handle single packer types -> returns single packer or sequence
	if (1==lua_gettop( L ))
	{
		p = t_pck_getpck( L, 1, &bo );
		return 1;
	}
	// Handle Array packer types
	if (2==lua_gettop( L ) && LUA_TNUMBER == lua_type( L, -1 ))
	{
		p = t_pck_mkarray( L );
		return 1;
	}
	// Handle everyting else ->Struct
	if (LUA_TTABLE == lua_type( L, -1 ))
	{
		p = t_pck_mkstruct( L, 1, lua_gettop( L ) );
		return 1;
	}
	else
	{
		p = t_pck_mksequence( L, 1, lua_gettop( L ), &bo );
		return 1;
	}

	return 1;
}


/** -------------------------------------------------------------------------
 * Creates the Packer from the Constructor.
 * Behaves differently based on arguments.
 * \param     L  lua state
 * \lparam    CLASS table Time
 * \lparam    length of buffer
 * \lparam    string buffer content initialized            OPTIONAL
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int lt_pck__Call (lua_State *L)
{
	lua_remove( L, 1 );    // remove the T.Buffer Class table
	return lt_pck_New( L );
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
 * \param   L  The lua state.
 * \lparam  ud     T.Pack.* instance.
 * \lreturn int    size in bytes.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_size( lua_State *L )
{
	struct t_pck *p = t_pck_getpckreader( L, 1, NULL );
	lua_pushinteger( L, t_pck_getsize( L, p, 0 ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set the default endian style of the T.Pack Constructor for fmt.
 * \param   L  The lua state.
 * \lparam  ud     T.Pack.* instance.
 * \lreturn int    size in bytes.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_defaultendian( lua_State *L )
{
	const char *endian = luaL_checkstring( L, -1 );
	if (*endian == 'n') /* native? */
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
 * \param   L  The lua state.
 * \lparam  ud     T.Pack.* instance.
 * \lreturn table  Reference table of all members.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_getir( lua_State *L )
{
	struct t_pck *p = t_pck_getpckreader( L, 1, NULL );
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
 * Read a Struct packer value.
 *          This can not simply return a packer/Struct type since it now has
 *          meta information about the position it is requested from.  For this
 *          the is a new datatype T.Pack.Result which carries type and position
 *          information
 * \param   L    The lua state.
 * \lparam  userdata T.Pack.Struct instance.
 * \lparam  key      string/integer.
 * \lreturn userdata Pack or Struct instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__index( lua_State *L )
{
	struct t_pcr *pr  = NULL;
	struct t_pck *pc  = t_pck_getpckreader( L, -2, &pr );
	struct t_pck *p;
	struct t_pcr *r;

	luaL_argcheck( L, pc->t > T_PCK_RAW, -2, "Trying to index Atomic "T_PCK_TYPE" type" );

	if (LUA_TNUMBER == lua_type( L, -1 ) &&
	      ((luaL_checkinteger( L, -1 ) > (int) pc->s) || (luaL_checkinteger( L, -1 ) < 1))
	   )
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( L );
		return 1;
	}
	// push empty reader on stack
	r = (struct t_pcr *) lua_newuserdata( L, sizeof( struct t_pcr ));
	r->o = (NULL == pr )? 0 : pr->o;  // recorded offset is 1 based -> don't add up
	// get idx table (struct) or packer type (array)
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m );
	//t_stackDump( L );
	// Stack: Struct,idx/name,Reader,idx/Packer
	if (LUA_TUSERDATA == lua_type( L, -1 ))        // T.Array
	{
		p     = t_pck_check_ud( L, -1, 1 );
		if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
		{
			lua_pop( L, 1 );
			p = t_pck_create_ud( L, p->t, p->s,
				((p->s * (luaL_checkinteger( L, -2 )-1)) % NB ) );
		}
		r->o += (((t_pck_getsize( L, p, 1 )) * (luaL_checkinteger( L, -3 )-1)) / NB);
	}
	else                                               // T.Pack.Struct/Sequence
	{
		if (! lua_tonumber( L, -3 ))               // T.Pack.Struct
		{
			lua_pushvalue( L, -3 );        // Stack: Struct,key,Reader,idx,key
			lua_rawget( L, -2 );           // Stack: Struct,key,Reader,idx,i
			lua_replace( L, -4 );          // Stack: Struct,i,Reader,idx
		}
		lua_rawgeti( L, -1, lua_tointeger( L, -3 ) + pc->s );  // Stack: Seq,i,Reader,idx,ofs
		r->o += luaL_checkinteger( L, -1);
		lua_pop( L, 1 );                                   // Stack: Seq,i,Reader,idx
		lua_rawgeti( L, -1, lua_tointeger( L, -3 ) );  // Stack: Seq,i,Reader,idx,Pack
		lua_remove( L, -2 );
		p =  t_pck_check_ud( L, -1, 1 );  // Stack: Seq,i,Reader,Pack
	}

	r->r  = luaL_ref( L, LUA_REGISTRYINDEX );   // Stack: Seq,i,Reader
	luaL_getmetatable( L, T_PCK_TYPE".Reader" );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an T.Pack.Struct ---> NOT ALLOWED.
 * \param   L    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__newindex( lua_State *L )
{
	struct t_pck *pc = t_pck_getpckreader( L, -3, NULL );

	luaL_argcheck( L, pc->t > T_PCK_RAW, -3, "Trying to index Atomic "T_PCK_TYPE" type" );

	return t_push_error( L, "Packers are static and can't be updated!" );
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   L lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_pck_iter( lua_State *L )
{
	struct t_pck *pc  = t_pck_check_ud( L, lua_upvalueindex( 1 ), 1);
	struct t_pcr *r;

	// get current index and increment
	int crs = lua_tointeger( L, lua_upvalueindex( 2 ) ) + 1;

	luaL_argcheck( L, pc->t > T_PCK_RAW, lua_upvalueindex( 1 ),
	   "Attempt to index atomic T.Pack type" );

	if (crs > (int) pc->s)
		return 0;
	else
	{
		lua_pushinteger( L, crs );
		lua_replace( L, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m );// Stack: func,nP,_idx
	if (T_PCK_STR == pc->t)                        // Get the name for a Struct value
		lua_rawgeti( L, -1 , crs + pc->s*2 );   // Stack: func,nP,_idx,nC
	else
		lua_pushinteger( L, crs );     // Stack: func,iP,_idx,iC
	r = (struct t_pcr *) lua_newuserdata( L, sizeof( struct t_pcr ));
	lua_rawgeti( L, -3 , crs+pc->s ); // Stack: func,xP,_idx,xC,Rd,ofs
	lua_rawgeti( L, -4 , crs );       // Stack: func,xP,_idx,xC,Rd,ofs,pack
	lua_remove( L, -5 );              // Stack: func,xP,xC,Rd,ofs,pack

	r->r = luaL_ref( L, LUA_REGISTRYINDEX );   // Stack: func,xP,xC,Rd,ofs
	r->o = lua_tointeger( L, lua_upvalueindex( 3 ) ) + luaL_checkinteger( L, -1 );
	lua_pop( L, 1 );                  // remove ofs
	luaL_getmetatable( L, T_PCK_TYPE".Reader" );
	lua_setmetatable( L, -2 );

	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   L lua Virtual Machine.
 * \lparam  iterator T.Pack.Struct.
 * \lreturn pos    position in t_buf.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pck__pairs( lua_State *L )
{
	struct t_pcr *pr = NULL;
	t_pck_getpckreader( L, -1, &pr );

	lua_pushnumber( L, 0 );
	lua_pushinteger( L, (NULL == pr) ? 0 : pr->o );  // preserve offset for iteration
	lua_pushcclosure( L, &t_pck_iter, 3 );
	lua_pushvalue( L, -1 );
	lua_pushnil( L );
	return 3;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Pack/Reader instance.
 * \param   L     The lua state.
 * \lparam  t_pack   the packer instance user_data.
 * \lreturn string    formatted string representing packer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__tostring( lua_State *L )
{
	struct t_pcr *pr = NULL;
	struct t_pck *pc = t_pck_getpckreader( L, -1, &pr );

	if (NULL == pr)
		lua_pushfstring( L, T_PCK_TYPE"." );
	else
		lua_pushfstring( L, T_PCK_TYPE".Reader[%d](", pr->o );
	t_pck_format( L, pc->t, pc->s, pc->m );
	lua_pushfstring( L, "): %p", pc );
	lua_concat( L, 4 );

	return 1;
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  L lua Virtual Machine.
 * \lparam ud    T.Pack.Struct.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__gc( lua_State *L )
{
	struct t_pcr *pr = NULL;
	struct t_pck *pc = t_pck_getpckreader( L, -1, &pr );
	if (NULL != pr)
		luaL_unref( L, LUA_REGISTRYINDEX, pr->r );
	if (NULL == pr && pc->t > T_PCK_RAW)
		luaL_unref( L, LUA_REGISTRYINDEX, pc->m );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Reader instance.
 * \param   L  lua Virtual Machine.
 * \lparam  ud     T.Pack.Struct/Reader instance.
 * \lreturn int    # of elements in T.Pack.Struct/Reader instance.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__len( lua_State *L )
{
	struct t_pck *pc = t_pck_getpckreader( L, -1, NULL );

	luaL_argcheck( L, pc->t > T_PCK_RAW, 1, "Attempt to get length of atomic "T_PCK_TYPE" type" );

	lua_pushinteger( L, pc->s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call helper to read from a T.Pack.Reader/Struct instance.
 * Leaves one element on the stack.
 * \param   L         lua Virtual Machine.
 * \param   stuct t_pck   T.Pack instance.
 * \param   char *        buffer to read from.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
int
t_pcr__callread( lua_State *L, struct t_pck *pc, const unsigned char *b )
{
	struct t_pck *p;      ///< packer currently processing
	size_t        sz = 0; ///< size of packer currently processing
	size_t         n;     /// iterator for complex types

	if (pc->t < T_PCK_ARR)         // handle atomic packer, return single value
	{
		return t_pck_read( L, pc,  b );
	}
	// for all others we need the p->m and a result table
	lua_createtable( L, pc->s, 0 );             //S:...,res
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m ); //S:...,res,idx
	if (pc->t == T_PCK_ARR)        // handle Array; return table
	{
		p = t_pck_check_ud( L, -1, 1 );
		sz = t_pck_getsize( L, p, 1 );       // size in bits!
		for (n=1; n <= pc->s; n++)
		{
			if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
			{
				lua_pop( L, 1 );
				p = t_pck_create_ud( L, p->t, p->s,
					((p->s * (n-1)) % NB ) );
			}
			t_pcr__callread( L, p, b + ((sz * (n-1)) /8) );       // Stack: ...,res,typ,val
			lua_rawseti( L, -3, n );
		}
		lua_pop( L, 1 );
		return 1;
	}
	if (pc->t == T_PCK_SEQ)       // handle Sequence, return table
	{
		for (n=1; n <= pc->s; n++)
		{
			lua_rawgeti( L, -1, n );           //S:...,res,idx,pack
			lua_rawgeti( L, -2, pc->s+n );     //S:...,res,idx,pack,ofs
			p = t_pck_check_ud( L, -2, 1 );
			t_pcr__callread( L, p, b + luaL_checkinteger( L, -1 ) );//S:...,res,idx,pack,ofs,val
			lua_rawseti( L, -5, n );           //S:...,res,idx,pack,ofs
			lua_pop( L, 2 );
		}
		lua_pop( L, 1 );
		return 1;
	}
	if (pc->t == T_PCK_STR)       // handle Struct, return table
	{
		for (n=1; n <= pc->s; n++)
		{
			lua_rawgeti( L, -1, n );           //S:...,res,idx,pack
			lua_rawgeti( L, -2, pc->s+n );     //S:...,res,idx,pack,ofs
			lua_rawgeti( L, -3, 2*pc->s+n );   //S:...,res,idx,pack,ofs,name
			p = t_pck_check_ud( L, -3, 1 );
			t_pcr__callread( L, p, b + luaL_checkinteger( L, -2 ) );//S:...,res,idx,pack,ofs,name,val
			lua_rawset( L, -5 );               // Stack: ...,res,idx,pack,ofs
			lua_pop( L, 2 );
		}
		lua_pop( L, 1 );
		return 1;
	}
	lua_pushnil( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Reader/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   L     lua Virtual Machine.
 * \lparam  ud        T.Pack.Reader instance.
 * \lparam  ud,string T.Buffer or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Reader.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pcr__call( lua_State *L )
{
	struct t_pcr  *pr = NULL;
	struct t_pck  *pc = t_pck_getpckreader( L, 1, &pr );

	size_t         o  = (NULL == pr) ? 0 : pr->o;

	struct t_buf  *buf;
	unsigned char *b;
	size_t         l;                   /// length of string or buffer overall
	luaL_argcheck( L,  2<=lua_gettop( L ) && lua_gettop( L )<=3, 2,
		"Calling an "T_PCK_TYPE".Reader takes 2 or 3 arguments!" );

	// are we reading/writing to from T.Buffer or Lua String
	if (lua_isuserdata( L, 2 ))      // T.Buffer
	{
		buf = t_buf_check_ud ( L, 2, 1 );
		luaL_argcheck( L,  buf->len >= o+t_pck_getsize( L, pc, 0 ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		b   =  &(buf->b[ o ]);
	}
	else
	{
		b   = (unsigned char *) luaL_checklstring( L, 2, &l );
		luaL_argcheck( L,  l >= o + t_pck_getsize( L, pc, 0 ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		luaL_argcheck( L,  2 == lua_gettop( L ), 2,
			"Can't write to a Lua String since they are immutable." );
		b   =  b + o;
	}

	if (2 == lua_gettop( L ))    // read from input
	{
		return t_pcr__callread( L, pc, b );
	}
	else                              // write to input
	{
		if (pc->t < T_PCK_ARR)          // handle atomic packer, return single value
		{
			return t_pck_write( L, pc, (unsigned char *) b );
		}
		else  // create a table ...
		{
			return t_push_error( L, "writing of complex types is not yet implemented");
		}
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
	  { "new"            , lt_pck_New }
	, { "size"           , lt_pck_size }
	, { "get_ref"        , lt_pck_getir }
	, { "setendian"      , lt_pck_defaultendian }
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_pck_m [] = {
	  { "__call"         , lt_pcr__call }
	, { "__index"        , lt_pck__index }
	, { "__newindex"     , lt_pck__newindex }
	, { "__pairs"        , lt_pck__pairs }
	, { "__gc"           , lt_pck__gc }
	, { "__len"          , lt_pck__len }
	, { "__tostring"     , lt_pck__tostring }
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * pushes the T.Pack.Reader library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
luaopen_t_pckr( lua_State *L )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( L, T_PCK_TYPE".Reader" );   // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack
	return 0;
}

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
	// T.Pack.Struct instance metatable
	luaL_newmetatable( L, T_PCK_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( L, t_pck_cf );
	luaL_newlib( L, t_pck_fm );
	lua_setmetatable( L, -2 );
	luaopen_t_pckr( L );
	return 1;
}
