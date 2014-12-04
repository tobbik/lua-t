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
// Macro helpers
#define HI_NIBBLE_GET(b)   (((b) >> 4) & 0xF)
#define LO_NIBBLE_GET(b)   ((b)  & 0xF)

#define HI_NIBBLE_SET(b,v) ( ((b) & 0x0F) | (((v)<<4) & 0xF0 ) )
#define LO_NIBBLE_SET(b,v) ( ((b) & 0xF0) | ( (v)     & 0x0F ) )

#define BIT_GET(b,n)       ( ((b) >> (7-(n))) & 0x01 )
#define BIT_SET(b,n,v)     \
	( (1==v)              ? \
	 ((b) | (  (0x01) << (7-(n))))    : \
	 ((b) & (~((0x01) << (7-(n))))) )


/* number of bits in a character */
#define NB                 CHAR_BIT

/* mask for one character (NB 1's) */
#define MC                 ((1 << NB) - 1)

/* size of a lua_Integer */
#define MXINT              ((int)sizeof(lua_Integer))

// Maximum bits that can be read or written
#define MXBIT              MXINT * NB

/* mask for all ones in last byte in a lua Integer */
#define HIGHERBYTE         ((lua_Unsigned)MC << (NB * (MXINT - 1)))


// Declaration because of circular dependency
static struct t_pck *t_pck_mksequence( lua_State *luaVM, int sp, int ep, size_t *bo );


// Function helpers
/**--------------------------------------------------------------------------
 * Read an integer of y bytes from a char buffer pointer
 * General helper function to read the value of an 64 bit integer from a char array
 * \param   sz         how many bytes to read.
 * \param   islittle   treat input as little endian?
 * \param   buf        pointer to char array to read from.
 * \return  val        integer value.
 * --------------------------------------------------------------------------*/
static lua_Unsigned
t_pck_rbytes( size_t sz, int islittle, const unsigned char * buf )
{
	size_t         i;
	lua_Unsigned   val = 0;                     ///< value for the read access
	unsigned char *set = (unsigned char*) &val; ///< char array to read bytewise into val
#ifndef IS_LITTLE_ENDIAN
	size_t         sz_l = sizeof( *val );       ///< size of the value in bytes

	for (i=sz_l; i<sz_l - sz -2; i--)
#else
	for (i=0 ; i<sz; i++)
#endif
		set[ i ] = buf[ (islittle) ? i :  sz-1-i ];

	return val;
}


/**--------------------------------------------------------------------------
 * Write an integer of y bytes to a char buffer pointer
 * General helper function to write the value of an 64 bit integer to a char array
 * \param  val         value to be written.
 * \param   sz         how many bytes to write.
 * \param   islittle   treat input as little endian?
 * \param   buf        pointer to char array to write to.
 * --------------------------------------------------------------------------*/
static void
t_pck_wbytes( lua_Unsigned val, size_t sz, int islittle, unsigned char * buf )
{
	size_t         i;
	unsigned char *set  = (unsigned char*) &val;  ///< char array to read bytewise into val
#ifndef IS_LITTLE_ENDIAN
	size_t         sz_l = sizeof( *val );         ///< size of the value in bytes

	for (i=sz_l; i<sz_l - sz -2; i--)
#else
	for (i=0 ; i<sz; i++)
#endif
		buf[ i ] = set[ (islittle) ? i :  sz-1-i ];
}


/**--------------------------------------------------------------------------
 * Read an integer of y bits from a char buffer with offset ofs.
 * \param   len  size in bits (1-64).
 * \param   ofs  offset   in bits (0-7).
 * \param  *buf  char buffer already on proper position
 * \return  val        integer value.
 * --------------------------------------------------------------------------*/
static lua_Unsigned
t_pck_rbits( size_t len, size_t ofs, const unsigned char * buf )
{
	lua_Unsigned val = t_pck_rbytes( (len+ofs-1)/8 +1, 0, buf );
#if PRINT_DEBUGS == 1
	printf("Read Val:    %016llX (%d)\nShift Left:  %016llX\nShift right: %016llX\n%d      %d\n",
			val, (len+ofs-1)/8 +1,
			(val << (MXBIT- ((len/8+1)*8) + ofs ) ),
			(val << (MXBIT- ((len/8+1)*8) + ofs ) ) >> (MXBIT - len),
			(MXBIT- ((len/8+1)*8) + ofs ), (MXBIT-len));
#endif
	return (val << (MXBIT- ((len/8+1)*8) + ofs ) ) >> (MXBIT - len);
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
	size_t     abit = (((len+ofs-1)/8)+1) * 8;

	msk = (0xFFFFFFFFFFFFFFFF << (MXBIT-len)) >> (MXBIT-abit+ofs);
	set = t_pck_rbytes( abit/8, 0, buf );
	set = (val << (abit-ofs-len)) | (set & ~msk);
	t_pck_wbytes( set, abit/8, 0, buf);

#if PRINT_DEBUGS == 1
	printf("Read: %016llX       \nLft:  %016lX       %d \nMsk:  %016lX       %ld\n"
	       "Nmsk: %016llX       \nval:  %016llX         \n"
	       "Sval: %016llX    %ld\nRslt: %016llX         \n",
			val,
			 0xFFFFFFFFFFFFFFFF <<   (MXBIT-len), (MXBIT-len),  /// Mask after left shift
			(0xFFFFFFFFFFFFFFFF <<	 (MXBIT-len)) >> (MXBIT-abit+ofs), (MXBIT-abit+ofs),
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
 * \param   luaVM lua Virtual Machine.
 * \param   struct t_pack.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 * -------------------------------------------------------------------------- */
int
t_pck_read( lua_State *luaVM, struct t_pck *p, const unsigned char *b )
{
	lua_Unsigned msk, val;
	switch( p->t )
	{
		case T_PCK_INT:
			msk = (lua_Unsigned) 1  << (p->s*NB - 1);
			val = t_pck_rbytes( p->s, p->m, b );
			//printf("%16llX  %16llX  %16llX  %16llX\n", msk, val, val^msk, (val^msk) - msk);
			lua_pushinteger( luaVM, (lua_Integer) ((val ^ msk) - msk) );
			break;
		case T_PCK_UNT:
			lua_pushinteger( luaVM, (1 == p->s)
				? (lua_Integer) *b
				: (lua_Integer) t_pck_rbytes( p->s, p->m, b ) );
			break;
		case T_PCK_BOL:
			lua_pushboolean( luaVM, BIT_GET( *b, p->m - 1 ) );
			break;
		case T_PCK_BTS:
			if (p->s == 1)
				lua_pushinteger( luaVM, BIT_GET( *b, p->m - 1 ) );
			else
			{
				msk = (lua_Unsigned) 1  << (p->s - 1);
				val = t_pck_rbits( p->s, p->m - 1, b );
				//printf("%16llX  %16llX  %16llX  %16llX\n", msk, val, val^msk, (val^msk) - msk);
				lua_pushinteger( luaVM, (lua_Integer) ((val ^ msk) - msk) );
			}
			break;
		case T_PCK_BTU:
			if (p->s == 1)
				lua_pushinteger( luaVM, BIT_GET( *b, p->m - 1 ) );
			else if (4 == p->s  && (1==p->m || 5==p->m))
				lua_pushinteger( luaVM, (5==p->m)
					? LO_NIBBLE_GET( *b )
					: HI_NIBBLE_GET( *b ) );
			else
				lua_pushinteger( luaVM, (lua_Integer) t_pck_rbits( p->s, p->m - 1, b ) );
			break;
		case T_PCK_RAW:
			lua_pushlstring( luaVM, (const char*) b, p->s );
			break;
		default:
			return t_push_error( luaVM, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Sets a value from stack to a char buffer according to paccker format.
 * \param  luaVM lua Virtual Machine.
 * \param  struct t_pack.
 * \param  unsigned char* char buffer to write to.
 * \lparam Lua value.
 *
 * return integer return code -0==success; !=0 means errors pushed to Lua stack
 *  -------------------------------------------------------------------------*/
int
t_pck_write( lua_State *luaVM, struct t_pck *p, unsigned char *b )
{
	lua_Integer     intVal;
	//lua_Number      fltVal;
	const char     *strVal;
	size_t          sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case T_PCK_INT:
		case T_PCK_UNT:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  0 == (intVal >> (p->s*8)) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (1==p->s)
				*b = (char) intVal;
			else
				t_pck_wbytes( intVal, p->s, p->m, b );
			break;
		case T_PCK_BOL:
			luaL_argcheck( luaVM,  lua_isboolean( luaVM, -1 ) , -1,
			   "value to pack must be boolean type" );
			*b = BIT_SET( *b, p->m - 1, lua_toboolean( luaVM, -1 ) );
			break;
		case T_PCK_BTS:
		case T_PCK_BTU:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  0 == (intVal >> p->s) , -1,
			   "value to pack must be smaller than the maximum value for the packer size" );
			if (p->s == 1)
				*b = BIT_SET( *b, p->m - 1, lua_toboolean( luaVM, -1 ) );
			else if (4 == p->s  && (1==p->m || 5==p->m))
				*b = (5==p->m)
					? LO_NIBBLE_SET( *b, (char) intVal )
					: HI_NIBBLE_SET( *b, (char) intVal );
			else
				t_pck_wbits( (lua_Unsigned) intVal, p->s, p->m - 1, b );
			break;
		case T_PCK_RAW:
			strVal = luaL_checklstring( luaVM, -1, &sL );
			luaL_argcheck( luaVM,  p->s < sL, -1, "String is to big for the field" );
			memcpy( b, strVal, sL );
			break;
		default:
			return t_push_error( luaVM, "Can't pack a value in unknown packer type" );
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
 * \param   luaVM     The lua state.
 * \param   t_pack    the packer instance struct.
 * \lreturn  leaves two strings on the Lua Stack.
 * --------------------------------------------------------------------------*/
void
t_pck_format( lua_State *luaVM, enum t_pck_t t, size_t s, int m )
{
	lua_pushfstring( luaVM, "%s", t_pck_t_lst[ t ] );
	switch( t )
	{
		case T_PCK_INT:
		case T_PCK_UNT:
			lua_pushfstring( luaVM, "%d%c", s, (1==m) ? 'L' : 'B' );
			break;
		case T_PCK_FLT:
			lua_pushfstring( luaVM, "%d", s );
			break;
		case T_PCK_BOL:
			lua_pushfstring( luaVM, "%d", m );
			break;
		case T_PCK_BTS:
		case T_PCK_BTU:
			lua_pushfstring( luaVM, "%d:%d", s, m );
			break;
		case T_PCK_RAW:
			lua_pushfstring( luaVM, "%d", s );
			break;
		case T_PCK_ARR:
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_pushfstring( luaVM, "[%d]", s );
			break;
		default:
			lua_pushfstring( luaVM, "UNKNOWN");
	}
}


/**--------------------------------------------------------------------------
 * See if requested type exists in T.Pack. otherwise create and register.
 * the format for a particular definition will never change. Hence, no need to
 * create them over and over again.  This approach saves memory.
 * \param     luaVM  lua state.
 * \param     enum   t_pck_t.
 * \param     size   number of elements.       
 * \param     mod parameter.
 * \return    struct t_pck* pointer. create a t_pack and push to LuaStack.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_create_ud( lua_State *luaVM, enum t_pck_t t, size_t s, int m )
{
	struct t_pck  __attribute__ ((unused)) *p;
	int                                     i;

	luaL_getsubtable( luaVM, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( luaVM, -1, "t" );
	lua_getfield( luaVM, -1, "Pack" );
	t_pck_format( luaVM, t, s, m ); lua_concat( luaVM, 2 );
	lua_rawget( luaVM, -2 );
	if (lua_isnil( luaVM, -1 ))    // havent found in cache -> create it
	{
		lua_pop( luaVM, 1 );        // pop the nil
		p = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ));
		p->t = t;
		p->s = s;
		p->m = m;

		luaL_getmetatable( luaVM, "T.Pack" );
		lua_setmetatable( luaVM, -2 );
		t_pck_format( luaVM, t, s, m ); lua_concat( luaVM, 2 );
		lua_pushvalue( luaVM, -2 );
		lua_rawset( luaVM, -4 );
	}
	p = t_pck_check_ud( luaVM, -1, 1 );
	for (i=0; i<3; i++)
		lua_remove( luaVM, -2 );

	return p;
}


/**--------------------------------------------------------------------------
 * Check if value on stack is T.Pack OR * T.Pack.Struct/Sequence/Array
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> error if fail
 * \lparam  userdata T.Pack/Struct on the stack.
 * \return  t_pck pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Pack" );
	if (NULL != ud)
		return (struct t_pck *) ud;
	if (check)
		luaL_argcheck( luaVM, ud != NULL, pos, "`T.Pack` expected" );
	return NULL;
}


/**--------------------------------------------------------------------------
 * Get the size of a packer of any type in bytes.
 * This will mainly be needed to calculate offsets when reading.
 * \param   luaVM    The lua state.
 * \param   struct t_pck.
 * \param   int    bits - boolean if bit resolution is needed.
 * \return  size in bytes.
 * TODO: return 0 no matter if even one item is of unknown length.
 * --------------------------------------------------------------------------*/
static size_t 
t_pck_getsize( lua_State *luaVM,  struct t_pck *p, int bits )
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
					: ((p->s + p->m -2)/8) + 1);
			break;
		case T_PCK_ARR:
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m ); // get packer
			s = p->s * t_pck_getsize( luaVM, t_pck_check_ud( luaVM, -1, 1 ), 1 );
			lua_pop( luaVM, 1 );
			return ((bits)
					? s
					: s/8);
			break;
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m ); // get table
			for (n = 1; n <= p->s; n++)
			{
				lua_rawgeti( luaVM, -1, n );
				s += t_pck_getsize( luaVM, t_pck_check_ud( luaVM, -1, 1 ), 1 );
				lua_pop( luaVM, 1 );
			}
			lua_pop( luaVM, 1 );
			return ((bits)
					? s
					: s/8);
			break;
		default:
			return 0;
	}
}


// ###########################################################################
//                                HELPERS from Lua 5.3 Source
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


/*
** Read an integer numeral and raises an error if it is larger
** than the maximum size for integers.
*/
static int
gnl( lua_State *luaVM, const char **fmt, int df, int max )
{
	int sz = gn( fmt, df );
	if (sz > max || sz <= 0)
		luaL_error( luaVM, "size (%d) out of limits [1,%d]", sz, max );
	return sz;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * Returns the Packer, or NULL if unsuccessful.
 * \param     luaVM  lua state.
 * \param     char*  format string pointer. moved by this function.
 * \param     int*   e pointer to current endianess.
 * \param     int*   bo pointer to current bit offset within byte.
 * \return    struct t_pck* pointer.
 * TODO: Deal with bit sized Packers:
 *			- Detect if we are in Bit sized type(o%8 !=0)
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
				m = gnl(L, f, 1+(*bo%8), 8);
				s = 1;
				break;
			case 'r':
				t = T_PCK_BTS;
				m = 1+(*bo%8);
				s = gnl(L, f, 1, MXBIT );
				break;
			case 'R':
				t = T_PCK_BTU;
				m = 1+(*bo%8);
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
		*bo += ((T_PCK_BTU==t || T_PCK_BTS==t || T_PCK_BOL == t) ? s : s*8);
	}
	return p;
}


/**--------------------------------------------------------------------------
 * Get T.Pack from a stack element.
 * Return Reader Pointer if requested.
 * \param  luaVM lua Virtual Machine.
 * \param  position on Lua stack.
 * \param  pointer to reader pointer.
 * \return pointer to t_pck struct*.
 * --------------------------------------------------------------------------*/
static inline struct t_pck
*t_pck_getpckreader( lua_State * luaVM, int pos, struct t_pcr **prp )
{
	void         *ud = luaL_testudata( luaVM, pos, "T.Pack.Reader" );
	struct t_pcr *pr = (NULL == ud) ? NULL : (struct t_pcr *) ud;
	struct t_pck *pc;
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( luaVM ) + pos + 1 : pos;

	//printf("%p\n", pr);
	if (NULL == pr)
		return t_pck_check_ud( luaVM, pos, 1 );
	else
	{
		if (NULL != prp)
			//*prp = *(&pr);
			*prp = pr;
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pr->r );
		pc = t_pck_check_ud( luaVM, -1, 1 );
		lua_replace( luaVM, pos );
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
 * \param   luaVM  The lua state.
 * \param   pos    position on stack.
 * \param   atom   boolean atomic packers only.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getpck( lua_State *luaVM, int pos, size_t *bo )
{
	struct t_pck *p = NULL; ///< packer
	int           l = IS_LITTLE_ENDIAN;
	int           n = 0;  ///< counter for packers created from fmt string
	int           t = lua_gettop( luaVM );  ///< top of stack before operations
	const char   *fmt;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( luaVM ) + pos + 1 : pos;

	if (lua_isuserdata( luaVM, pos ))
	{
		p    = t_pck_getpckreader( luaVM, pos, NULL );
		*bo += t_pck_getsize( luaVM, p, 1 );
	}
	else
	{
		fmt = luaL_checkstring( luaVM, pos );
		p   = t_pck_getoption( luaVM, &fmt, &l, bo );
		while (NULL != p )
		{
			n++;
			p   = t_pck_getoption( luaVM, &fmt, &l, bo );
		}
		// TODO: actually create the packers and calculate positions
		if (1 < n)
		{
			//t_stackDump( luaVM );
			p =  t_pck_mksequence( luaVM, t+1, lua_gettop( luaVM ), bo );
			//t_stackDump( luaVM );
		}
		else
			p = t_pck_check_ud( luaVM, -1, 1);
		lua_replace( luaVM, pos );
	}
	//t_stackDump( luaVM );
	//printf("%d\n",n);
	return p;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  T.Pack, T.Pack.Struct, T.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mkarray( lua_State *luaVM )
{
	//t_stackDump( luaVM );
	size_t            bo = 0;
	struct t_pck  __attribute__ ((unused))   *p  = t_pck_getpck( luaVM, -2, &bo );  ///< packer
	struct t_pck     *ap;     ///< array userdata to be created

	//t_stackDump( luaVM );
	ap    = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	ap->t = T_PCK_ARR;
	ap->s = luaL_checkinteger( luaVM, -2 );      // how many elements in the array

	lua_pushvalue( luaVM, -3 );  // Stack: Pack,n,Array,Pack
	ap->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register packer table

	luaL_getmetatable( luaVM, "T.Pack" );
	lua_setmetatable( luaVM, -2 ) ;

	return ap;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mksequence( lua_State *luaVM, int sp, int ep, size_t *bo )
{
	size_t        n=1;    ///< iterator for going through the arguments
	size_t        o=0;    ///< byte offset within the sequence
	struct t_pck *p;      ///< temporary packer/struct for iteration
	struct t_pck *sq;     ///< the userdata this constructor creates

	sq     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	sq->t  = T_PCK_SEQ;
	sq->s  = (ep-sp)+1;

	// create and populate index table
	lua_newtable( luaVM );                  // Stack: fmt,Seq,idx
	while (n <= sq->s)
	{
		p = t_pck_getpck( luaVM, sp, bo );
		lua_pushvalue( luaVM, sp );          // Stack: fmt,Seq,idx,Pack
		lua_pushinteger( luaVM, o/8 );       // Stack: fmt,Seq,idx,Pack,ofs
		lua_rawseti( luaVM, -3, n + sq->s ); // Stack: fmt,Seq,idx,Pack     idx[n+i] = offset
		lua_rawseti( luaVM, -2, n );         // Stack: fmt,Seq,idx,         idx[i]   = Pack
		o += t_pck_getsize( luaVM, p, 1 );
		n++;
		lua_remove( luaVM, sp );
	}
	sq->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return sq;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \lparam  ... multiple of type  table { name = T.Pack}.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pck
*t_pck_mkstruct( lua_State *luaVM, int sp, int ep )
{
	size_t        n  = 1;  ///< iterator for going through the arguments
	size_t        o  = 0;  ///< byte offset within the sequence
	size_t        bo = 0;  ///< bit  offset within the sequence
	struct t_pck *p;       ///< temporary packer/struct for iteration
	struct t_pck *st;      ///< the userdata this constructor creates

	st     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	st->t  = T_PCK_STR;
	st->s  = (ep-sp)+1;

	// create and populate index table
	lua_newtable( luaVM );                  // Stack: fmt,Seq,idx
	while (n <= st->s)
	{
		luaL_argcheck( luaVM, lua_istable( luaVM, sp ), n,
			"Arguments must be tables with single key/T.Pack pair" );
		// Stack gymnastic:
		lua_pushnil( luaVM );
		if (! lua_next( luaVM, sp ))         // Stack: ...,Struct,idx,name,Pack
			luaL_error( luaVM, "the table argument must contain one key/value pair." );
		// check if name is already used!
		lua_pushvalue( luaVM, -2 );          // Stack: ...,Struct,idx,name,Pack,name
		lua_rawget( luaVM, -4 );             // Stack: ...,Struct,idx,name,Pack,nil?
		if (! lua_isnoneornil( luaVM, -1 ))
			luaL_error( luaVM, "All elements in T.Pack.Struct must have unique key." );
		lua_pop( luaVM, 1 );                 // Stack: ...,Struct,idx,name,Pack
		p = t_pck_getpck( luaVM, -1, &bo );  // allow T.Pack or T.Pack.Struct
		// populate idx table
		lua_pushinteger( luaVM, o/8 );       // Stack: ...,Seq,idx,name,Pack,ofs
		lua_rawseti( luaVM, -4, n + st->s ); // Stack: ...,Seq,idx,name,Pack        idx[n+i] = offset
		lua_rawseti( luaVM, -3, n );         // Stack: ...,Seq,idx,name             idx[i] = Pack
		lua_pushvalue( luaVM, -1 );          // Stack: ...,Seq,idx,name,name
		lua_rawseti( luaVM, -3, st->s*2+n ); // Stack: ...,Seq,idx,name             idx[2n+i] = name
		lua_pushinteger( luaVM, n);          // Stack: ...,Seq,idx,name,i
		lua_rawset( luaVM, -3 );             // Stack: ...,Seq,idx                  idx[name] = i
		o += t_pck_getsize( luaVM, p, 1 );
		n++;
		lua_remove( luaVM, sp );
	}
	st->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

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
 * \param     luaVM  lua state.
 * \lparam    fmt    string.
 *      or
 * \lparam    T.Pack elements.
 *      or
 * \lparam    {name=T.Pack}, ...  name value pairs.
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int
lt_pck_New( lua_State *luaVM )
{
	struct t_pck  __attribute__ ((unused)) *p;
	size_t                                  bo = 0;

	// Handle single packer types -> returns single packer or sequence
	if (1==lua_gettop( luaVM ))
	{
		p = t_pck_getpck( luaVM, 1, &bo );
		return 1;
	}
	// Handle Array packer types
	if (2==lua_gettop( luaVM ) && LUA_TNUMBER == lua_type( luaVM, -1 ))
	{
		p = t_pck_mkarray( luaVM );
		return 1;
	}
	// Handle everyting else ->Struct
	if (LUA_TTABLE == lua_type( luaVM, -1 ))
	{
		p = t_pck_mkstruct( luaVM, 1, lua_gettop( luaVM ) );
		return 1;
	}
	else
	{
		p = t_pck_mksequence( luaVM, 1, lua_gettop( luaVM ), &bo );
		return 1;
	}

	return 1;
}


/** -------------------------------------------------------------------------
 * Creates the Packer from the Constructor.
 * Behaves differently based on arguments.
 * \param     luaVM  lua state
 * \lparam    CLASS table Time
 * \lparam    length of buffer
 * \lparam    string buffer content initialized            OPTIONAL
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int lt_pck__Call (lua_State *luaVM)
{
	lua_remove( luaVM, 1 );    // remove the T.Buffer Class table
	return lt_pck_New( luaVM );
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
 * \param   luaVM  The lua state.
 * \lparam  ud     T.Pack.* instance.
 * \lreturn int    size in bytes.
 * \return  int    The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_size( lua_State *luaVM )
{
	struct t_pck *p = t_pck_getpckreader( luaVM, 1, NULL );
	lua_pushinteger( luaVM, t_pck_getsize( luaVM, p, 0 ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * DEBUG: Get internal reference table from a Struct/Packer.
 * \param   luaVM  The lua state.
 * \lparam  ud     T.Pack.* instance.
 * \lreturn table  Reference table of all members.
 * \return  int    The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_getir( lua_State *luaVM )
{
	struct t_pck *p = t_pck_getpckreader( luaVM, 1, NULL );
	if (p->t > T_PCK_RAW && LUA_NOREF != p->m)
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m );
	else
		lua_pushnil( luaVM );
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
 * \param   luaVM    The lua state.
 * \lparam  userdata T.Pack.Struct instance.
 * \lparam  key      string/integer.
 * \lreturn userdata Pack or Struct instance.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__index( lua_State *luaVM )
{
	struct t_pcr *pr  = NULL;
	struct t_pck *pc  = t_pck_getpckreader( luaVM, -2, &pr );
	struct t_pck *p;
	struct t_pcr *r;

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, -2, "Trying to index Atomic T.Pack type" );

	if (LUA_TNUMBER == lua_type( luaVM, -1 ) &&
	      ((luaL_checkinteger( luaVM, -1 ) > (int) pc->s) || (luaL_checkinteger( luaVM, -1 ) < 1))
	   )
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( luaVM );
		return 1;
	}
	// push empty reader on stack
	r = (struct t_pcr *) lua_newuserdata( luaVM, sizeof( struct t_pcr ));
	r->o = (NULL == pr )? 0 : pr->o;  // recorded offset is 1 based -> don't add up
	// get idx table (struct) or packer type (array)
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->m );
	// Stack: Struct,idx/name,Reader,idx/Packer
	if (LUA_TUSERDATA == lua_type( luaVM, -1 ))        // T.Array
	{
		p     = t_pck_check_ud( luaVM, -1, 1 );
		if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
		{
			lua_pop( luaVM, 1 );
			p = t_pck_create_ud( luaVM, p->t, p->s,
				((p->s * (luaL_checkinteger( luaVM, -2 )-1)) % 8 ) + 1 );
		}
		r->o += (((t_pck_getsize( luaVM, p, 1 )) * (luaL_checkinteger( luaVM, -3 )-1)) / 8);
	}
	else                                               // T.Struct/Sequence
	{
		if (! lua_tonumber( luaVM, -3 ))               // T.Struct
		{
			lua_pushvalue( luaVM, -3 );        // Stack: Struct,key,Reader,idx,key
			lua_rawget( luaVM, -2 );           // Stack: Struct,key,Reader,idx,i
			lua_replace( luaVM, -4 );          // Stack: Struct,i,Reader,idx
		}
		lua_rawgeti( luaVM, -1, lua_tointeger( luaVM, -3 ) + pc->s );  // Stack: Seq,i,Reader,idx,ofs
		r->o += luaL_checkinteger( luaVM, -1);
		lua_pop( luaVM, 1 );                                   // Stack: Seq,i,Reader,idx
		lua_rawgeti( luaVM, -1, lua_tointeger( luaVM, -3 ) );  // Stack: Seq,i,Reader,idx,Pack
		lua_remove( luaVM, -2 );
		p =  t_pck_check_ud( luaVM, -1, 1 );  // Stack: Seq,i,Reader,Pack
	}

	r->r  = luaL_ref( luaVM, LUA_REGISTRYINDEX );   // Stack: Seq,i,Reader
	luaL_getmetatable( luaVM, "T.Pack.Reader" );
	lua_setmetatable( luaVM, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an T.Pack.Struct ---> NOT ALLOWED.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__newindex( lua_State *luaVM )
{
	struct t_pck *pc = t_pck_getpckreader( luaVM, -3, NULL );

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, -3, "Trying to index Atomic T.Pack type" );

	return t_push_error( luaVM, "Packers are static and can't be updated!" );
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int
t_pck_iter( lua_State *luaVM )
{
	struct t_pck *pc  = t_pck_check_ud( luaVM, lua_upvalueindex( 1 ), 1);
	struct t_pcr *r;

	// get current index and increment
	int crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) ) + 1;

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, lua_upvalueindex( 1 ),
	   "Attempt to index atomic T.Pack type" );

	if (crs > (int) pc->s)
		return 0;
	else
	{
		lua_pushinteger( luaVM, crs );
		lua_replace( luaVM, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->m );// Stack: func,nP,_idx
	if (T_PCK_STR == pc->t)                        // Get the name for a Struct value
		lua_rawgeti( luaVM, -1 , crs + pc->s*2 );   // Stack: func,nP,_idx,nC
	else
		lua_pushinteger( luaVM, crs );     // Stack: func,iP,_idx,iC
	r = (struct t_pcr *) lua_newuserdata( luaVM, sizeof( struct t_pcr ));
	lua_rawgeti( luaVM, -3 , crs+pc->s ); // Stack: func,xP,_idx,xC,Rd,ofs
	lua_rawgeti( luaVM, -4 , crs );       // Stack: func,xP,_idx,xC,Rd,ofs,pack
	lua_remove( luaVM, -5 );              // Stack: func,xP,xC,Rd,ofs,pack

	r->r = luaL_ref( luaVM, LUA_REGISTRYINDEX );   // Stack: func,xP,xC,Rd,ofs
	r->o = lua_tointeger( luaVM, lua_upvalueindex( 3 ) ) + luaL_checkinteger( luaVM, -1 );
	lua_pop( luaVM, 1 );                  // remove ofs
	luaL_getmetatable( luaVM, "T.Pack.Reader" );
	lua_setmetatable( luaVM, -2 );

	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator T.Pack.Struct.
 * \lreturn pos    position in t_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pck__pairs( lua_State *luaVM )
{
	struct t_pcr *pr = NULL;
	t_pck_getpckreader( luaVM, -1, &pr );

	lua_pushnumber( luaVM, 0 );
	lua_pushinteger( luaVM, (NULL == pr) ? 0 : pr->o );  // preserve offset for iteration
	lua_pushcclosure( luaVM, &t_pck_iter, 3 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Pack/Reader instance.
 * \param   luaVM     The lua state.
 * \lparam  t_pack   the packer instance user_data.
 * \lreturn string    formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pck__tostring( lua_State *luaVM )
{
	struct t_pcr *pr = NULL;
	struct t_pck *pc = t_pck_getpckreader( luaVM, -1, &pr );
	//printf("%p\n", pr);

	if (NULL == pr)
		lua_pushfstring( luaVM, "T.Pack." );
	else
		lua_pushfstring( luaVM, "T.Pack.Reader[%d](", pr->o );
	t_pck_format( luaVM, pc->t, pc->s, pc->m );
	lua_pushfstring( luaVM, "): %p", pc );
	lua_concat( luaVM, 4 );

	return 1;
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  luaVM lua Virtual Machine.
 * \lparam ud    T.Pack.Struct.
 * \return int   # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__gc( lua_State *luaVM )
{
	struct t_pcr *pr = NULL;
	struct t_pck *pc = t_pck_getpckreader( luaVM, -1, &pr );
	if (NULL != pr)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, pr->r );
	if (pc->t > T_PCK_RAW)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, pc->m );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Reader instance.
 * \param   luaVM  lua Virtual Machine.
 * \lparam  ud     T.Pack.Struct/Reader instance.
 * \lreturn int    # of elements in T.Pack.Struct/Reader instance.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__len( lua_State *luaVM )
{
	struct t_pck *pc = t_pck_getpckreader( luaVM, -1, NULL );

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, 1, "Attempt to get length of atomic T.Pack type" );

	lua_pushinteger( luaVM, pc->s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call helper to read from a T.Pack.Reader/Struct instance.
 * Leaves one element on the stack.
 * \param   luaVM         lua Virtual Machine.
 * \param   stuct t_pck   T.Pack instance.
 * \param   char *        buffer to read from.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
int
t_pcr__callread( lua_State *luaVM, struct t_pck *pc, const unsigned char *b )
{
	struct t_pck *p;      ///< packer currently processing
	size_t        sz = 0; ///< size of packer currently processing
	size_t         n;     /// iterator for complex types

	if (pc->t < T_PCK_ARR)         // handle atomic packer, return single value
	{
		return t_pck_read( luaVM, pc,  b );
	}
	// for all others we need the p->m and a result table
	lua_createtable( luaVM, pc->s, 0 );             //S:...,res
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->m ); //S:...,res,idx
	if (pc->t == T_PCK_ARR)        // handle Array; return table
	{
		p = t_pck_check_ud( luaVM, -1, 1 );
		sz = t_pck_getsize( luaVM, p, 1 );       // size in bits!
		for (n=1; n <= pc->s; n++)
		{
			if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
			{
				lua_pop( luaVM, 1 );
				p = t_pck_create_ud( luaVM, p->t, p->s,
					((p->s * (n-1)) % 8 ) + 1 );
			}
			t_pcr__callread( luaVM, p, b + ((sz * (n-1)) /8) );       // Stack: ...,res,typ,val
			lua_rawseti( luaVM, -3, n );
		}
		lua_pop( luaVM, 1 );
		return 1;
	}
	if (pc->t == T_PCK_SEQ)       // handle Sequence, return table
	{
		for (n=1; n <= pc->s; n++)
		{
			lua_rawgeti( luaVM, -1, n );           //S:...,res,idx,pack
			lua_rawgeti( luaVM, -2, pc->s+n );     //S:...,res,idx,pack,ofs
			p = t_pck_check_ud( luaVM, -2, 1 );
			t_pcr__callread( luaVM, p, b + luaL_checkinteger( luaVM, -1 ) );//S:...,res,idx,pack,ofs,val
			lua_rawseti( luaVM, -5, n );           //S:...,res,idx,pack,ofs
			lua_pop( luaVM, 2 );
		}
		lua_pop( luaVM, 1 );
		return 1;
	}
	if (pc->t == T_PCK_STR)       // handle Struct, return table
	{
		for (n=1; n <= pc->s; n++)
		{
			lua_rawgeti( luaVM, -1, n );           //S:...,res,idx,pack
			lua_rawgeti( luaVM, -2, pc->s+n );     //S:...,res,idx,pack,ofs
			lua_rawgeti( luaVM, -3, 2*pc->s+n );   //S:...,res,idx,pack,ofs,name
			p = t_pck_check_ud( luaVM, -3, 1 );
			t_pcr__callread( luaVM, p, b + luaL_checkinteger( luaVM, -2 ) );//S:...,res,idx,pack,ofs,name,val
			lua_rawset( luaVM, -5 );               // Stack: ...,res,idx,pack,ofs
			lua_pop( luaVM, 2 );
		}
		lua_pop( luaVM, 1 );
		return 1;
	}
	lua_pushnil( luaVM );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Reader/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  ud        T.Pack.Reader instance.
 * \lparam  ud,string T.Buffer or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Reader.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pcr__call( lua_State *luaVM )
{
	struct t_pcr  *pr = NULL;
	struct t_pck  *pc = t_pck_getpckreader( luaVM, 1, &pr );

	size_t         o  = (NULL == pr) ? 0 : pr->o;

	struct t_buf  *buf;
	unsigned char *b;
	size_t         l;                   /// length of string or buffer overall
	luaL_argcheck( luaVM,  2<=lua_gettop( luaVM ) && lua_gettop( luaVM )<=3, 2,
		"Calling an T.Pack.Reader takes 2 or 3 arguments!" );

	// are we reading/writing to from T.Buffer or Lua String
	if (lua_isuserdata( luaVM, 2 ))      // T.Buffer
	{
		buf = t_buf_check_ud ( luaVM, 2, 1 );
		luaL_argcheck( luaVM,  buf->len >= o+t_pck_getsize( luaVM, pc, 0 ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		b   =  &(buf->b[ o ]);
	}
	else
	{
		b   = (unsigned char *) luaL_checklstring( luaVM, 2, &l );
		luaL_argcheck( luaVM,  l >= o + t_pck_getsize( luaVM, pc, 0 ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		luaL_argcheck( luaVM,  2 == lua_gettop( luaVM ), 2,
			"Can't write to a Lua String since they are immutable." );
		b   =  b + o;
	}

	if (2 == lua_gettop( luaVM ))    // read from input
	{
		return t_pcr__callread( luaVM, pc, b );
	}
	else                              // write to input
	{
		if (pc->t < T_PCK_ARR)          // handle atomic packer, return single value
		{
			return t_pck_write( luaVM, pc, (unsigned char *) b );
		}
		else  // create a table ...
		{
			return t_push_error( luaVM, "writing of complex types is not yet implemented");
		}
	}

	return 0;
}



/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_fm [] = {
	{"__call",        lt_pck__Call},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_cf [] = {
	{"new",       lt_pck_New},
	{"size",      lt_pck_size},
	{"get_ref",   lt_pck_getir},
	{NULL,    NULL}
};



/**--------------------------------------------------------------------------
 * \brief   pushes the T.Pack.Reader library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
luaopen_t_pckr( lua_State *luaVM )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "T.Pack.Reader" );   // stack: functions meta
	lua_pushcfunction( luaVM, lt_pck__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pck__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_pck__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lt_pck__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pck__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lt_pck__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pushcfunction( luaVM, lt_pcr__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

/**--------------------------------------------------------------------------
 * \brief   pushes the T.Pack library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_pck( lua_State *luaVM )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "T.Pack" );   // stack: functions meta
	lua_pushcfunction( luaVM, lt_pck__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pck__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_pck__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lt_pck__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pck__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lt_pck__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pushcfunction( luaVM, lt_pcr__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( luaVM, t_pck_cf );
	luaL_newlib( luaVM, t_pck_fm );
	lua_setmetatable( luaVM, -2 );
	luaopen_t_pckr( luaVM );
	return 1;
}


// ==========================================================================
// preserve float packing/unpacking code from Lua 5.3 alpha
// Lua 5.3 went with lpack support

/*
// translate a relative string position: negative means back from end
static lua_Integer posrelat (lua_Integer pos, size_t len) {
  if (pos >= 0) return pos;
  else if (0u - (size_t)pos > len) return 0;
  else return (lua_Integer)len + pos + 1;
}


static union {
  int dummy;
  char little;  // true iff machine is little endian
} const nativeendian = {1};


static int getendian (lua_State *L, int arg) {
  const char *endian = luaL_optstring(L, arg,
                             (nativeendian.little ? "l" : "b"));
  if (*endian == 'n')  // native?
    return nativeendian.little;
  luaL_argcheck(L, *endian == 'l' || *endian == 'b', arg,
                   "endianness must be 'l'/'b'/'n'");
  return (*endian == 'l');
}

static void correctendianness (lua_State *L, char *b, int size, int endianarg) {
  int endian = getendian(L, endianarg);
  if (endian != nativeendian.little) {  // not native endianness?
    int i = 0;
    while (i < --size) {
      char temp = b[i];
      b[i++] = b[size];
      b[size] = temp;
    }
  }
}


static int getfloatsize (lua_State *L, int arg) {
  const char *size = luaL_optstring(L, arg, "n");
  if (*size == 'n') return sizeof(lua_Number);
  luaL_argcheck(L, *size == 'd' || *size == 'f', arg,
                   "size must be 'f'/'d'/'n'");
  return (*size == 'd' ? sizeof(double) : sizeof(float));
}


static int dumpfloat_l (lua_State *L) {
  float f;  double d;
  char *pn;  // pointer to number
  lua_Number n = luaL_checknumber(L, 1);
  int size = getfloatsize(L, 2);
  if (size == sizeof(lua_Number))
    pn = (char*)&n;
  else if (size == sizeof(float)) {
    f = (float)n;
    pn = (char*)&f;
  }  
  else {  // native lua_Number may be neither float nor double
    lua_assert(size == sizeof(double));
    d = (double)n;
    pn = (char*)&d;
  }
  correctendianness(L, pn, size, 3);
  lua_pushlstring(L, pn, size);
  return 1;
}


static int undumpfloat_l (lua_State *L) {
  lua_Number res;
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  lua_Integer pos = posrelat(luaL_optinteger(L, 2, 1), len);
  int size = getfloatsize(L, 3);
  luaL_argcheck(L, 1 <= pos && (size_t)pos + size - 1 <= len, 1,
                   "string too short");
  if (size == sizeof(lua_Number)) {
    memcpy(&res, s + pos - 1, size); 
    correctendianness(L, (char*)&res, size, 4);
  }
  else if (size == sizeof(float)) {
    float f;
    memcpy(&f, s + pos - 1, size); 
    correctendianness(L, (char*)&f, size, 4);
    res = (lua_Number)f;
  }  
  else {  // native lua_Number may be neither float nor double
    double d;
    lua_assert(size == sizeof(double));
    memcpy(&d, s + pos - 1, size); 
    correctendianness(L, (char*)&d, size, 4);
    res = (lua_Number)d;
  }
  lua_pushnumber(L, res);
  return 1;
}
*/



