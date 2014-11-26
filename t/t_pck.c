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

// =========+Buffer accessor Helpers
//
//
// Macro helper functions
#define HI_NIBBLE_GET(b)   (((b) >> 4) & 0xF)
#define LO_NIBBLE_GET(b)   ((b)  & 0xF)

#define HI_NIBBLE_SET(b,v) ( ((b) & 0x0F) | (((v)<<4) & 0xF0 ) )
#define LO_NIBBLE_SET(b,v) ( ((b) & 0xF0) | ( (v)     & 0x0F ) )

#define BIT_GET(b,n)       ( ((b) >> (7-(n))) & 0x01 )
#define BIT_SET(b,n,v)     \
	( (1==v)              ? \
	 ((b) | (  (0x01) << (7-(n))))    : \
	 ((b) & (~((0x01) << (7-(n))))) )


/**--------------------------------------------------------------------------
 * Read an integer of y bytes from a char buffer pointer
 * General helper function to read the value of an 64 bit integer from a char array
 * \param   sz         how many bytes to read.
 * \param   islittle   treat input as little endian?
 * \param   buf        pointer to char array to read from.
 * \return  val        integer value.
 * --------------------------------------------------------------------------*/
static inline uint64_t
t_pck_rbytes( size_t sz, int islittle, const unsigned char * buf )
{
	size_t         i;
	uint64_t       val = 0;                     ///< value for the read access
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
static inline void
t_pck_wbytes( uint64_t val, size_t sz, int islittle, unsigned char * buf )
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
static inline uint64_t
t_pck_rbits( size_t len, size_t ofs, const unsigned char * buf )
{
	uint64_t val = t_pck_rbytes( (len+ofs-1)/8 +1, 0, buf );

#if PRINT_DEBUGS == 1
	printf("Read Val:    %016llX (%d)\nShift Left:  %016llX\nShift right: %016llX\n%d      %d\n",
			val, (len+ofs-1)/8 +1,
			(val << (64- ((len/8+1)*8) + ofs ) ),
			(val << (64- ((len/8+1)*8) + ofs ) ) >> (64 - len),
			(64- ((len/8+1)*8) + ofs ), (64-len));
#endif
	return  (val << (64- ((len/8+1)*8) + ofs ) ) >> (64 - len);
}


/**--------------------------------------------------------------------------
 * Write an integer of y bits from ta char buffer with offset ofs.
 * \param   val  the val gets written to.
 * \param   len  size in bits (1-64).
 * \param   ofs  offset   in bits (0-7).
 * \param  *buf  char buffer already on proper position
 * --------------------------------------------------------------------------*/
static inline void
t_pck_wbits( uint64_t val, size_t len, size_t ofs, unsigned char * buf )
{
	uint64_t   set = 0;                           ///< value for the read access
	uint64_t   msk = 0;                           ///< mask
	/// how many bit are in all the bytes needed for the conversion
	size_t     abit = (((len+ofs-1)/8)+1) * 8;

	msk = (0xFFFFFFFFFFFFFFFF  << (64-len)) >> (64-abit+ofs);
	set = t_pck_rbytes( abit/8, 0, buf );
	set = (val << (abit-ofs-len)) | (set & ~msk);
	t_pck_wbytes( set, abit/8, 0, buf);

#if PRINT_DEBUGS == 1
	printf("Read: %016llX       \nLft:  %016lX       %d \nMsk:  %016lX       %ld\n"
	       "Nmsk: %016llX       \nval:  %016llX         \n"
	       "Sval: %016llX    %ld\nRslt: %016llX         \n",
			val,
			 0xFFFFFFFFFFFFFFFF <<   (64-len), (64-len),  /// Mask after left shift
			(0xFFFFFFFFFFFFFFFF <<	 (64-len)) >> (64-abit+ofs), (64-abit+ofs),
			set & ~msk,
			val,
			 val << (abit-ofs-len),  abit-ofs-len,
			(val << (abit-ofs-len)) | (set & ~msk)
			);
#endif
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC t_pck API========================
// Reader and writer for packer data
/**--------------------------------------------------------------------------
 * reads a value from the packer and pushes it onto the Lua stack.
 * \param   luaVM lua Virtual Machine.
 * \param   struct t_pack.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_pck_read( lua_State *luaVM, struct t_pck *p, const unsigned char *b)
{
	switch( p->t )
	{
		case T_PCK_INT:
			lua_pushinteger( luaVM, (1 == p->s)
				? (lua_Integer)  *b
				: (lua_Integer)   t_pck_rbytes( p->s, p->m, b ) );
			break;
		case T_PCK_UNT:
			lua_pushinteger( luaVM, (1 == p->s)
				? (lua_Unsigned) *b
				: (lua_Unsigned)  t_pck_rbytes( p->s, p->m, b ) );
			break;
		case T_PCK_BIT:
			if (p->s == 1)
				lua_pushboolean( luaVM, BIT_GET( *b, p->m - 1 ) );
			else if (4 == p->s  && (1==p->m || 5==p->m))
				lua_pushinteger( luaVM, (5==p->m) ? LO_NIBBLE_GET( *b ) : HI_NIBBLE_GET( *b ) );
			else
				lua_pushinteger( luaVM, (lua_Integer) t_pck_rbits( p->s, p->m - 1 , b ) );
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
			if (1==p->s)
			{
				luaL_argcheck( luaVM,  0<= intVal && intVal<=255, -1,
				   "value to pack must be greater 0 and less than 255");
				*b = (char) intVal;
			}
			else
			{
				luaL_argcheck( luaVM,  0 == (intVal >> (p->s*8)) , -1,
				   "value to pack must be smaller than the maximum value for the packer size");
				t_pck_wbytes( (uint64_t) intVal, p->s, p->m, b );
			}
			break;
		case T_PCK_BIT:
			if (p->s == 1)
				*b = BIT_SET( *b, p->m - 1, lua_toboolean( luaVM, -1 ) );
			else
			{
				intVal = luaL_checkinteger( luaVM, -1 );
				luaL_argcheck( luaVM,  0 == (intVal >> (p->s)) , -1,
				   "value to pack must be smaller than the maximum value for the packer size");
				t_pck_wbits( (uint64_t) intVal, p->s, p->m - 1, b );
			}
			break;
		case T_PCK_RAW:
			strVal = luaL_checklstring( luaVM, -1, &sL );
			luaL_argcheck( luaVM,  p->s < sL, -1,
			              "String is to big for the field" );
			memcpy( b, strVal, sL );
			break;
		default:
			return t_push_error( luaVM, "Can't pack a value in unknown packer type" );
	}
	return 0;
}


// ###########################################################################
//                                HELPERS
/** -------------------------------------------------------------------------
 * See if int represents a character which is a digit.
 * \param     int c
 * \return    boolean 0:flase - 1:true
 *  -------------------------------------------------------------------------*/
static int
is_digit( int c ) { return '0' <= c && c<='9'; }


/** -------------------------------------------------------------------------
 * reads from string until input5 is not numeric any more.
 * \param     char** format string
 * \param     int    default value
 * \return    int    read numeric value
 *  -------------------------------------------------------------------------*/
static int
get_num( const char **fmt, int df )
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
 * See if requested type exists in t.Pack. otherwise create and register.
 * the format for a particular definition will never change. Hence, no need to
 * create them over and over again.  This approach saves memory.
 * \param     luaVM  lua state.
 * \param     enum   t_pck_t.
 * \param     size   number of elements.       
 * \param     mod parameter.
 * \return    struct t_pck* pointer.
 *  -------------------------------------------------------------------------*/
struct t_pck
*t_pck_fnd( lua_State *luaVM, enum t_pck_t t, size_t s, int m)
{
	struct t_pck  __attribute__ ((unused)) *p;
	int                                     i;

	luaL_getsubtable( luaVM, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( luaVM, -1, "t" );
	lua_getfield( luaVM, -1, "Pack" );
	t_pck_format( luaVM, t, s, m ); lua_concat( luaVM, 2 );
	lua_rawget( luaVM, -2 );
	if (lua_isnil( luaVM, -1 ))
	{
		lua_pop( luaVM, 1 );
		t_pck_format( luaVM, t, s, m ); lua_concat( luaVM, 2 );
		p = t_pck_create_ud( luaVM, t );
		p->s = s; p->m = m;
		lua_rawset( luaVM, -3);
		t_pck_format( luaVM, t, s, m ); lua_concat( luaVM, 2 );
		lua_rawget( luaVM, -2);
	}
	else
	{
		p = t_pck_check_ud( luaVM, -1, 1 );
	}
	for(i=0; i<3; i++)
		lua_remove( luaVM, -2 );

	return p;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * \param     luaVM  lua state.
 * \param     char*  format string pointer. moved by this function.
 * \param     int*   e pointer to current endianess.
 * \param     int*   o pointer to current offset within fmt.
 * \return    struct t_pck* pointer.
 *  -------------------------------------------------------------------------*/
struct t_pck
*t_pck_getoption( lua_State *luaVM, const char **f, int *e, int *o )
{
	int           opt = *((*f)++);
	int           m;
	size_t        s;
	enum t_pck_t  t;
	struct t_pck *p   = NULL;
	switch (opt)
	{
		case 'b': t = T_PCK_INT; s =                     1; m = (1==*e); break;
		case 'B': t = T_PCK_UNT; s =                     1; m = (1==*e); break;
		case 'h': t = T_PCK_INT; s = sizeof(       short ); m = (1==*e); break;
		case 'H': t = T_PCK_INT; s = sizeof(       short ); m = (1==*e); break;
		case 'l': t = T_PCK_INT; s = sizeof(        long ); m = (1==*e); break;
		case 'L': t = T_PCK_INT; s = sizeof(        long ); m = (1==*e); break;
		case 'j': t = T_PCK_INT; s = sizeof( lua_Integer ); m = (1==*e); break;
		case 'J': t = T_PCK_INT; s = sizeof( lua_Integer ); m = (1==*e); break;
		case 'T': t = T_PCK_INT; s = sizeof( lua_Integer ); m = (1==*e); break;
		case 'f': t = T_PCK_INT; s = sizeof(       float ); m = (1==*e); break;
		case 'd': t = T_PCK_INT; s = sizeof(      double ); m = (1==*e); break;
		case 'n': t = T_PCK_INT; s = sizeof(  lua_Number ); m = (1==*e); break;
		case 'i': t = T_PCK_INT; s = get_num( f, sizeof( int ) ); m = (1==*e); break;
		case 'I': t = T_PCK_UNT; s = get_num( f, sizeof( int ) ); m = (1==*e); break;
		case 'c': t = T_PCK_RAW; s = get_num( f, 1 )      ; m = 0;       break;
		case 'r': t = T_PCK_BIT; s =                     1; m = 1+(*o%8); break;
		case 'R': t = T_PCK_BIT; s = get_num( f, 1 )      ; m = 1+(*o%8); break;

		case '<': *e = 1; return NULL; break;
		case '>': *e = 0; return NULL; break;
		default:
			luaL_error( luaVM, "can't do that bro");
			return NULL;
	}

	p   = t_pck_fnd( luaVM, t, s, m );
	*o  = *o + ((T_PCK_BIT==t) ? s : s*8);
	return p;
}


//###########################################################################
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
//###########################################################################
/** -------------------------------------------------------------------------
 * \brief     creates a packerfrom the function call
 * \param     luaVM  lua state
 * \lparam    fmt string
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int
lt_pck_New( lua_State *luaVM )
{
	int                                     is_little = IS_LITTLE_ENDIAN;
	int                                     offset    = 0;
	const char                             *fmt = luaL_checkstring( luaVM, 1 );
	struct t_pck  __attribute__ ((unused)) *p;

	p = t_pck_getoption( luaVM, &fmt, &is_little, &offset );
	while (NULL == p)  p = t_pck_getoption( luaVM, &fmt, &is_little, &offset );
	return 1;
}



/** -------------------------------------------------------------------------
 * creates the buffer from the Constructor.
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


/**--------------------------------------------------------------------------
 * create a t_pack and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_pack*  pointer to the  t_pack struct
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_create_ud( lua_State *luaVM, enum t_pck_t t)
{
	struct t_pck  *p;
	p = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ));
	p->t  = t;

	luaL_getmetatable( luaVM, "T.Pack" );
	lua_setmetatable( luaVM, -2 );
	return p;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_pack struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct t_pack* pointer to t_pack struct
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Pack" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Pack` expected" );
	return (NULL==ud) ? NULL : (struct t_pck *) ud;
}


/**--------------------------------------------------------------------------
 * reads a value, unpacks it and pushes it onto the Lua stack.
 * \param   luaVM  lua Virtual Machine.
 * \lparam  struct t_pack.
 * \lreturn value  unpacked value according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pck_unpack( lua_State *luaVM )
{
	struct t_pck *p   = t_pck_check_ud( luaVM, 1, 1);
	size_t        sL;
	const char   *buf = luaL_checklstring( luaVM, 2, &sL );
	if (sL != p->s)
		return t_push_error( luaVM, "Can only unpack data of the size suitable for this packers size" );

	return t_pck_read( luaVM, p, (const unsigned char *) buf );
}


/**--------------------------------------------------------------------------
 * reads in a Lua value and packs it according to packer format. Return str to Lua Stack
 * \param   luaVM lua Virtual Machine.
 * \lparam  struct t_pack.
 * \lparam  Lua value.
 * \lreturn string packed value according to packer format.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pck_pack( lua_State *luaVM )
{
	struct t_pck *p      = t_pck_check_ud( luaVM, 1, 1 );
	luaL_Buffer   lB;
	char         *buffer;
	int           retVal; ///< return value to evaluate the succes of write operation

	luaL_buffinit( luaVM, &lB );
	buffer = luaL_prepbuffsize( &lB, p->s );
	memset( buffer, 0, p->s * sizeof( char ) );

	if ((retVal = t_pck_write( luaVM, p, (unsigned char *) buffer )) != 0)
	{
		return retVal;
	}
	else
	{
		luaL_pushresultsize( &lB, p->s );
		return 1;
	}
}


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
	//struct t_pckr *pr = t_pckr_check_ud( luaVM, 1, 0 );
	//struct t_pck  *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, -1, 1 ) : pr->p;
	struct t_pck *p = t_pck_check_ud( luaVM, 1, 1 );
	lua_pushinteger( luaVM, p->s );
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
	//struct t_pckr *pr = t_pckr_check_ud( luaVM, 1, 0 );
	//struct t_pck  *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, -1, 1 ) : pr->p;
	struct t_pck *p = t_pck_check_ud( luaVM, 1, 1 );
	if (p->t > T_PCK_RAW && LUA_NOREF != p->m)
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m );
	else
		lua_pushnil( luaVM );
	return 1;
}


/**--------------------------------------------------------------------------
 * __tostring helper that prints the packer type.
 * \param   luaVM     The lua state.
 * \param   t_pack    the packer instance struct.
 * \lreturn  leaves two strings on the Lua Stack.
 * --------------------------------------------------------------------------*/
void
// t_pck_format( lua_State *luaVM, struct t_pck *p )
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
		case T_PCK_BIT:
			lua_pushfstring( luaVM, "%d_%d",  s, m );
			break;
		case T_PCK_RAW:
			lua_pushfstring( luaVM, "%d", s );
			break;
		default:
			lua_pushfstring( luaVM, "[%d:%d]", s, m );
	}
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a packer instance.
 * \param   luaVM     The lua state.
 * \lparam  t_pack   the packer instance user_data.
 * \lreturn string    formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pck__tostring( lua_State *luaVM )
{
	struct t_pck *p = t_pck_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM, "T.Pack." );
	t_pck_format( luaVM, p->t, p->s, p->m );
	lua_pushfstring( luaVM, ": %p", p );
	lua_concat( luaVM, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a packer instance.
 * \param   luaVM     The lua state.
 * \lparam  t_pack    the packer instance user_data.
 * \lreturn string    formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pck__len( lua_State *luaVM )
{
	struct t_pck *p = t_pck_check_ud( luaVM, 1, 1 );
	lua_pushinteger( luaVM, p->s );
	return 1;
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
//	{"Array",     lt_pckc_Array},
//	{"Sequence",  lt_pckc_Sequence},
//	{"Struct",    lt_pckc_Struct},
	{"size",      lt_pck_size},
	{"get_ref",   lt_pck_getir},
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * \brief   the packer library definition
 *          assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_pck_m [] = {
	// new implementation
	{"pack",      lt_pck_pack},
	{"unpack",    lt_pck_unpack},
	{NULL,    NULL}
};


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
	// T.Pack instance metatable
	luaL_newmetatable( luaVM, "T.Pack" );   // stack: functions meta
	luaL_newlib( luaVM, t_pck_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pck__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pck__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( luaVM, t_pck_cf );
	luaL_newlib( luaVM, t_pck_fm );
	lua_setmetatable( luaVM, -2 );
	//luaopen_t_pckc( luaVM );
	//luaopen_t_pckr( luaVM );
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



