/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pcc.c
 * \brief     OOP wrapper for Packer definitions
 *            Allows for packing/unpacking numeric values to binary streams
 *            can work stand alone or as helper for Combinators
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset

#include "t.h"
#include "t_pcc.h"

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
t_pcc_rbytes( size_t sz, int islittle, const unsigned char * buf )
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
t_pcc_wbytes( uint64_t val, size_t sz, int islittle, unsigned char * buf )
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
t_pcc_rbits( size_t len, size_t ofs, const unsigned char * buf )
{
	uint64_t val = t_pcc_rbytes( (len+ofs-1)/8 +1, 0, buf );

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
t_pcc_wbits( uint64_t val, size_t len, size_t ofs, unsigned char * buf )
{
	uint64_t   set = 0;                           ///< value for the read access
	uint64_t   msk = 0;                           ///< mask
	/// how many bit are in all the bytes needed for the conversion
	size_t     abit = (((len+ofs-1)/8)+1) * 8;

	msk = (0xFFFFFFFFFFFFFFFF  << (64-len)) >> (64-abit+ofs);
	set = t_pcc_rbytes( abit/8, 0, buf );
	set = (val << (abit-ofs-len)) | (set & ~msk);
	t_pcc_wbytes( set, abit/8, 0, buf);

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
// ================================= GENERIC t_pcc API========================
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
t_pcc_read( lua_State *luaVM, struct t_pcc *p, const unsigned char *b)
{
	switch( p->t )
	{
		case T_PCC_INT:
			if (1 == p->s)
				lua_pushinteger( luaVM, (p->m > 0)
					? (lua_Integer)   *b
					: (lua_Unsigned)  *b );
			else
				lua_pushinteger( luaVM, (p->m > 0)
					? (lua_Integer)   t_pcc_rbytes( p->s, ( 1 == p->m), b )
					: (lua_Unsigned)  t_pcc_rbytes( p->s, (-1 == p->m), b ) );
			break;
		case T_PCC_BIT:
			if (p->s == 1)
				lua_pushboolean( luaVM, BIT_GET( *b, p->m - 1 ) );
			else if (4 == p->s  && (1==p->m || 5==p->m))
				lua_pushinteger( luaVM, (5==p->m) ? LO_NIBBLE_GET( *b ) : HI_NIBBLE_GET( *b ) );
			else
				lua_pushinteger( luaVM, (lua_Integer) t_pcc_rbits( p->s, p->m - 1 , b ) );
		case T_PCC_RAW:
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
t_pcc_write( lua_State *luaVM, struct t_pcc *p, unsigned char *b )
{
	lua_Integer     intVal;
	//lua_Number      fltVal;
	const char     *strVal;
	size_t          sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case T_PCC_INT:
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
				t_pcc_wbytes( (uint64_t) intVal, p->s, (p->m > 0), b );
			}
			break;
		case T_PCC_BIT:
			if (p->s == 1)
				*b = BIT_SET( *b, p->m - 1, lua_toboolean( luaVM, -1 ) );
			else
			{
				intVal = luaL_checkinteger( luaVM, -1 );
				luaL_argcheck( luaVM,  0 == (intVal >> (p->s)) , -1,
				   "value to pack must be smaller than the maximum value for the packer size");
				t_pcc_wbits( (uint64_t) intVal, p->s, p->m - 1, b );
			}
			break;
		case T_PCC_RAW:
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

/// ----------------------------HELPERS
static int
is_digit( int c ) { return '0' <= c && c<='9'; }

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


//###########################################################################
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
//###########################################################################
static struct t_pcc
*t_pcc_getoption( lua_State *luaVM, const char **f, int *e )
{
	int           opt = *((*f)++);
	struct t_pcc *p   = NULL;
	switch (opt)
	{
		case 'b': lua_pushstring( luaVM,                        "ByteS"    ); break;
		case 'B': lua_pushstring( luaVM,                        "ByteU"    ); break;
		case 'h': lua_pushstring( luaVM, (1==*e) ? "ShortSL"  : "ShortSB"  ); break;
		case 'H': lua_pushstring( luaVM, (1==*e) ? "ShortUL"  : "ShortUB"  ); break;
		case 'l': lua_pushstring( luaVM, (1==*e) ? "LongSL"   : "LongSB"   ); break;
		case 'L': lua_pushstring( luaVM, (1==*e) ? "LongUL"   : "LongUB"   ); break;
		case 'j': lua_pushstring( luaVM, (1==*e) ? "LuaIntSL" : "LuaIntSB" ); break;
		case 'J': lua_pushstring( luaVM, (1==*e) ? "LuaIntUL" : "LuaIntUB" ); break;
		case 'T': lua_pushstring( luaVM, (1==*e) ? "SizeTL"   : "SizeTB"   ); break;
		case 'f': lua_pushstring( luaVM,                        "Float"    ); break;
		case 'd': lua_pushstring( luaVM,                        "Double"   ); break;
		case 'n': lua_pushstring( luaVM,                        "LuaNum"   ); break;
		case 'i':
			lua_pushfstring( luaVM, (1==*e) ? "IntSL%d" : "IntSB%D", get_num( f, sizeof( int ) ) );
			break;
		case 'I':
			lua_pushfstring( luaVM, (1==*e) ? "IntUL%d" : "IntUB%D", get_num( f, sizeof( int ) ) );
			break;
		case 'c':
			p = t_pcc_create_ud( luaVM, T_PCC_RAW );
			p->s = get_num( f, 1 );
			break;
		case 'r': lua_pushfstring( luaVM, "BIT%d", get_num( f, sizeof( int ) ) ); break;
		case 'R':
			p = t_pcc_create_ud( luaVM, T_PCC_BIT );
			p->s = get_num( f, 1 );
			break;
		case '<': *e = 1; return NULL; break;
		case '>': *e = 0; return NULL; break;
		default:
			luaL_error( luaVM, "can't do that bro");
	}
	if (NULL==p)
	{
		lua_rawget( luaVM, -2 );
		return t_pcc_check_ud( luaVM, -1, 1 );
	}
	else
		return p;
}


/** -------------------------------------------------------------------------
 * \brief     creates a packerfrom the function call
 * \param     luaVM  lua state
 * \lparam    fmt string
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int lt_pcc_New( lua_State *luaVM )
{
	int                                     is_little = IS_LITTLE_ENDIAN;
	const char                             *fmt = luaL_checkstring( luaVM, 1 );
	struct t_pcc  __attribute__ ((unused)) *p;

	luaL_getsubtable( luaVM, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( luaVM, -1, "t" );
	lua_getfield( luaVM, -1, "Pcc" );
	p = t_pcc_getoption( luaVM, &fmt, &is_little );
	while (NULL == p)  p = t_pcc_getoption( luaVM, &fmt, &is_little );
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
static int lt_pcc__Call (lua_State *luaVM)
{
	lua_remove( luaVM, 1 );    // remove the T.Buffer Class table
	return lt_pcc_New( luaVM );
}


/**--------------------------------------------------------------------------
 * create a t_pack and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_pack*  pointer to the  t_pack struct
 * --------------------------------------------------------------------------*/
struct t_pcc
*t_pcc_create_ud( lua_State *luaVM, enum t_pcc_t t)
{
	struct t_pcc  *p;
	p = (struct t_pcc *) lua_newuserdata( luaVM, sizeof( struct t_pcc ));
	p->t  = t;

	luaL_getmetatable( luaVM, "T.Pcc" );
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
struct t_pcc
*t_pcc_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Pcc" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Pcc` expected" );
	return (NULL==ud) ? NULL : (struct t_pcc *) ud;
}


/**--------------------------------------------------------------------------
 * reads a value, unpacks it and pushes it onto the Lua stack.
 * \param   luaVM  lua Virtual Machine.
 * \lparam  struct t_pack.
 * \lreturn value  unpacked value according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_pcc_unpack( lua_State *luaVM )
{
	struct t_pcc *p   = t_pcc_check_ud( luaVM, 1, 1);
	size_t        sL;
	const char   *buf = luaL_checklstring( luaVM, 2, &sL );
	if (sL != p->s)
		return t_push_error( luaVM, "Can only unpack data of the size suitable for this packers size" );

	return t_pcc_read( luaVM, p, (const unsigned char *) buf );
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
lt_pcc_pack( lua_State *luaVM )
{
	struct t_pcc *p      = t_pcc_check_ud( luaVM, 1, 1 );
	luaL_Buffer   lB;
	char         *buffer;
	int           retVal; ///< return value to evaluate the succes of write operation

	luaL_buffinit( luaVM, &lB );
	buffer = luaL_prepbuffsize( &lB, p->s );
	memset( buffer, 0, p->s * sizeof( char ) );

	if ((retVal = t_pcc_write( luaVM, p, (unsigned char *) buffer )) != 0)
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
lt_pcc_size( lua_State *luaVM )
{
	//struct t_pccr *pr = t_pccr_check_ud( luaVM, 1, 0 );
	//struct t_pcc  *pc = (NULL == pr) ? t_pccc_check_ud( luaVM, -1, 1 ) : pr->p;
	struct t_pcc *p = t_pcc_check_ud( luaVM, 1, 1 );
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
lt_pcc_getir( lua_State *luaVM )
{
	//struct t_pccr *pr = t_pccr_check_ud( luaVM, 1, 0 );
	//struct t_pcc  *pc = (NULL == pr) ? t_pccc_check_ud( luaVM, -1, 1 ) : pr->p;
	struct t_pcc *p = t_pcc_check_ud( luaVM, 1, 1 );
	if (p->t > T_PCC_RAW && LUA_NOREF != p->m)
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
t_pcc_format( lua_State *luaVM, struct t_pcc *p )
{
	lua_pushfstring( luaVM, "%s", t_pcc_t_lst[ p->t ] );
	switch( p->t )
	{
		case T_PCC_INT:
			lua_pushfstring( luaVM, "%c%c[%d]",
				(p->m > 0) ? 'U' : 'S',
				(p->m*p->m == 4) ? 'B' : 'L',
				p->s );
			break;
		case T_PCC_FLT:
			lua_pushfstring( luaVM, "[%d]", p->s );
			break;
		case T_PCC_BIT:
			lua_pushfstring( luaVM, "%d[%d]",  p->m, p->s );
			break;
		case T_PCC_RAW:
			lua_pushfstring( luaVM, "[%d]", p->s );
			break;
		default:
			lua_pushfstring( luaVM, "[%d:%d]", p->s, p->m );
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
lt_pcc__tostring( lua_State *luaVM )
{
	struct t_pcc *p = t_pcc_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM, "T.Pcc." );
	t_pcc_format( luaVM, p );
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
lt_pcc__len( lua_State *luaVM )
{
	struct t_pcc *p = t_pcc_check_ud( luaVM, 1, 1 );
	lua_pushinteger( luaVM, p->s );
	return 1;
}


/**--------------------------------------------------------------------------
 * Exports all types of T.Pack as global variables
 * \param   luaVM     The lua state.
 * \lparam  t_pack    the packer instance user_data.
 * \lreturn string    formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pcc_export( lua_State *luaVM )
{
	luaL_getsubtable( luaVM, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( luaVM, -1, "t" );
	lua_getfield( luaVM, -1, "Pcc" );
	lua_pushnil( luaVM );
	while (lua_next( luaVM, -2 ))
	{                            // Stack, _LOADED,t,Pack,name,func
		if(90 < *(lua_tostring( luaVM, -2 )))  // only uppercase elements
		{
			lua_pop( luaVM, 1 );
			continue;
		}
		else
			lua_setglobal( luaVM, lua_tostring( luaVM, -2 ) );
	}
	lua_pop( luaVM, 3 );
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pcc_fm [] = {
	{"__call",        lt_pcc__Call},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pcc_cf [] = {
//	{"Array",     lt_pckc_Array},
//	{"Sequence",  lt_pckc_Sequence},
//	{"Struct",    lt_pckc_Struct},
	{"size",      lt_pcc_size},
	{"export",    lt_pcc_export},
	{"get_ref",   lt_pcc_getir},
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * \brief   the packer library definition
 *          assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_pcc_m [] = {
	// new implementation
	{"pack",      lt_pcc_pack},
	{"unpack",    lt_pcc_unpack},
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
luaopen_t_pcc( lua_State *luaVM )
{
	
	int           i;                   /// iterator for type creation
	struct t_pcc *t;                   /// type pointer for type creation
	// T.Pack instance metatable
	luaL_newmetatable( luaVM, "T.Pcc" );   // stack: functions meta
	luaL_newlib( luaVM, t_pcc_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pcc__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pcc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( luaVM, t_pcc_cf );
	// register static types
	// IntSL,IntUB, Byte., Short.., Long.., LuaInteger..
	for (i=1; i<=8; i++)
	{
		lua_pushfstring( luaVM, "IntSL%d", i );
		t    = t_pcc_create_ud( luaVM, T_PCC_INT );
		t->s = i;
		t->m = -1;
		lua_rawset( luaVM, -3 );
		lua_pushfstring( luaVM, "IntUL%d", i );
		t     = t_pcc_create_ud( luaVM, T_PCC_INT );
		t->s = i;
		t->m = 1;
		lua_rawset( luaVM, -3 );
		lua_pushfstring( luaVM, "IntSB%d", i );
		t    = t_pcc_create_ud( luaVM, T_PCC_INT );
		t->s = i;
		t->m = -2;
		lua_rawset( luaVM, -3 );
		lua_pushfstring( luaVM, "IntUB%d", i );
		t     = t_pcc_create_ud( luaVM, T_PCC_INT );
		t->s = i;
		t->m = 2;
		lua_rawset( luaVM, -3 );
		if (1==i)
		{
			lua_pushstring( luaVM, "ByteS" );
			lua_pushfstring( luaVM, "IntSL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "ByteU" );
			lua_pushfstring( luaVM, "IntUL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
		}
		if (sizeof( short )==i)
		{
			lua_pushstring( luaVM, "ShortSL" );
			lua_pushfstring( luaVM, "IntSL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "ShortUL" );
			lua_pushfstring( luaVM, "IntUL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
			lua_pushstring( luaVM, "ShortSB" );
			lua_pushfstring( luaVM, "IntSB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "ShortUB" );
			lua_pushfstring( luaVM, "IntUB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
		}
		if (sizeof( long )==i)
		{
			lua_pushstring( luaVM, "LongSL" );
			lua_pushfstring( luaVM, "IntSL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "LongUL" );
			lua_pushfstring( luaVM, "IntUL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
			lua_pushstring( luaVM, "LongSB" );
			lua_pushfstring( luaVM, "IntSB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "LongUB" );
			lua_pushfstring( luaVM, "IntUB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
		}
		if (sizeof( lua_Integer )==i)
		{
			lua_pushstring( luaVM, "LuaIntegerSL" );
			lua_pushfstring( luaVM, "IntSL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "LuaIntegerUL" );
			lua_pushfstring( luaVM, "IntUL%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
			lua_pushstring( luaVM, "LuaIntegerSB" );
			lua_pushfstring( luaVM, "IntSB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -3 );
			lua_pushstring( luaVM, "LuaIntegerUB" );
			lua_pushfstring( luaVM, "IntUB%d", i );
			lua_rawget( luaVM, -3 );
			lua_rawset( luaVM, -4 );
		}
	}
	// Bit1 ..Bit8
	for (i=1; i<=8; i++)
	{
		lua_pushfstring( luaVM, "Bit%d", i );
		t = t_pcc_create_ud( luaVM, T_PCC_BIT );
		t->s  = 1;
		t->m  = i;
		lua_rawset( luaVM, -3 );
	}
	// NibbleL
	lua_pushstring( luaVM, "NibbleL" );
	t = t_pcc_create_ud( luaVM, T_PCC_BIT );
	t->s  = 4;
	t->m  = 1;
	lua_rawset( luaVM, -3 );
	// NibbleH
	lua_pushstring( luaVM, "NibbleH" );
	t = t_pcc_create_ud( luaVM, T_PCC_BIT );
	t->s  = 4;
	t->m  = 5;
	lua_rawset( luaVM, -3 );
	// Byte
	lua_pushstring( luaVM, "ByteS" );
	t = t_pcc_create_ud( luaVM, T_PCC_INT );
	t->s  = 1;
	t->m  = -1;
	lua_rawset( luaVM, -3 );
	lua_pushstring( luaVM, "ByteU" );
	t = t_pcc_create_ud( luaVM, T_PCC_INT );
	t->s  = 1;
	t->m  = 1;
	lua_rawset( luaVM, -3 );
	luaL_newlib( luaVM, t_pcc_fm );
	lua_setmetatable( luaVM, -2 );
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



