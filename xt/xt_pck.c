/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_pck.c
 * \brief     OOP wrapper for Packer definitions
 *            Allows for packing/unpacking numeric values to binary streams
 *            can work stand alone or as helper for Combinators
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */


#include <string.h>               // memset

#include "xt.h"
#include "xt_buf.h"

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
static inline uint64_t xt_buf_readbytes( size_t sz, int islittle, const unsigned char * buf )
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
		set[ i ] = (islittle) ? buf[ i ]: buf[ sz-1-i ];
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
static inline void xt_buf_writebytes( uint64_t val, size_t sz, int islittle, unsigned char * buf )
{
	size_t         i;
	unsigned char *set  = (unsigned char*) &val;  ///< char array to read bytewise into val
#ifndef IS_LITTLE_ENDIAN
	size_t         sz_l = sizeof( *val );        ///< size of the value in bytes

	for (i=sz_l; i<sz_l - sz -2; i--)
#else
	for (i=0 ; i<sz; i++)
#endif
		buf[ i ] = (islittle) ? set[ i ] : set[ sz-1-i ];
}


/**--------------------------------------------------------------------------
 * Read an integer of y bits from a char buffer with offset ofs.
 * \param   len  size in bits (1-64).
 * \param   ofs  offset   in bits (0-7).
 * \param  *buf  char buffer already on proper position
 * \return  val        integer value.
 * --------------------------------------------------------------------------*/
static inline uint64_t xt_buf_readbits( size_t len, size_t ofs, const unsigned char * buf )
{
	uint64_t val = xt_buf_readbytes( (len+ofs-1)/8 +1, 0, buf );

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
static inline void xt_buf_writebits( uint64_t val, size_t len, size_t ofs, unsigned char * buf )
{
	uint64_t   read = 0;                           ///< value for the read access
	uint64_t   msk  = 0;                           ///< mask
	/// how many bit are in all the bytes needed for the conversion
	size_t     abit = (((len+ofs-1)/8)+1) * 8;

	msk  = (0xFFFFFFFFFFFFFFFF  << (64-len)) >> (64-abit+ofs);
	read = xt_buf_readbytes( abit/8, 0, buf );
	read = (val << (abit-ofs-len)) | (read & ~msk);
	xt_buf_writebytes( read, abit/8, 0, buf);

#if PRINT_DEBUGS == 1
	printf("Read: %016llX       \nLft:  %016lX       %d \nMsk:  %016lX       %ld\n"
	       "Nmsk: %016llX       \nval:  %016llX         \n"
	       "Sval: %016llX    %ld\nRslt: %016llX         \n",
			read,
			 0xFFFFFFFFFFFFFFFF <<   (64-len), (64-len),  /// Mask after left shift
			(0xFFFFFFFFFFFFFFFF <<	 (64-len)) >> (64-abit+ofs), (64-abit+ofs),
			read & ~msk,
			val,
			 val << (abit-ofs-len),  abit-ofs-len,
			(val << (abit-ofs-len)) | (read & ~msk)
			);
#endif
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC xt_pck API========================
// Reader and writer for packer data
/**--------------------------------------------------------------------------
 * reads a value from the packer and pushes it onto the Lua stack.
 * \param   luaVM lua Virtual Machine.
 * \param   struct xt_pack.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int xt_pck_read( lua_State *luaVM, struct xt_pck *p, const unsigned char *b)
{
	switch( p->t )
	{
		case XT_PCK_INTL:
		case XT_PCK_INTB:
			lua_pushinteger( luaVM, (lua_Integer) xt_buf_readbytes( p->sz, p->t, b ) );
			break;
		case XT_PCK_BYTE:
			lua_pushinteger( luaVM, (lua_Integer) *b );
			break;
		case XT_PCK_NBL:
			lua_pushinteger( luaVM, (p->oB > 4) ? LO_NIBBLE_GET( *b ) : HI_NIBBLE_GET( *b ) );
			break;
		case XT_PCK_BIT:
			lua_pushboolean( luaVM, BIT_GET( *b, p->oB - 1 ) );
			break;
		case XT_PCK_BITS:
			lua_pushinteger( luaVM, (lua_Integer) xt_buf_readbits( p->lB, p->oB - 1 , b ) );
			break;
		case XT_PCK_STR:
			lua_pushlstring( luaVM, (const char*) b, p->sz );
			break;
		default:
			return xt_push_error( luaVM, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Sets a value from stack to a char buffer according to paccker format.
 * \param  luaVM lua Virtual Machine.
 * \param  struct xt_pack.
 * \param  unsigned char* char buffer to write to.
 * \lparam Lua value.
 *
 * return integer return code -0==success; !=0 means errors pushed to Lua stack
 *  -------------------------------------------------------------------------*/
int xt_pck_write( lua_State *luaVM, struct xt_pck *p, unsigned char *b )
{
	lua_Integer     intVal;
	//lua_Number      fltVal;
	const char     *strVal;
	size_t          sL;

	// TODO: size check values if they fit the packer size
	switch( p->t )
	{
		case XT_PCK_INTL:
		case XT_PCK_INTB:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  0 == (intVal >> (p->sz*8)) , -1,
			              "value to pack must be smaller than the maximum value for the packer size");
			xt_buf_writebytes( (uint64_t) intVal, p->sz, p->t, b );
			break;
		case XT_PCK_BYTE:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  0<= intVal && intVal<=255, -1,
			              "value to pack must be greaer 0 and less than 255");
			*b = (char) intVal;
			break;
		case XT_PCK_NBL:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  intVal  <  0x01 << 0x0F, -1,
			              "value to pack nibble must be smaller than 16 (0x0F)");
			*b = (p->oB > 4) ? LO_NIBBLE_SET( *b, (char) intVal ) : HI_NIBBLE_SET( *b, (char) intVal ) ;
			break;
		case XT_PCK_BIT:
			*b = BIT_SET( *b, p->oB - 1, lua_toboolean( luaVM, -1 ) );
			break;
		case XT_PCK_BITS:
			intVal = luaL_checkinteger( luaVM, -1 );
			luaL_argcheck( luaVM,  0 == (intVal >> (p->sz*8)) , -1,
			              "value to pack must be smaller than the maximum value for the packer size");
			xt_buf_writebits( (uint64_t) intVal, p->lB, p->oB - 1, b );
			break;
		case XT_PCK_STR:
			strVal = luaL_checklstring( luaVM, -1, &sL );
			if (p->sz < sL)
			luaL_argcheck( luaVM,  p->sz < sL, -1,
			              "String is to big for the field" );
			memcpy( b, strVal, sL );
			break;
		default:
			return xt_push_error( luaVM, "Can't pack a value in unknown packer type" );
	}
	return 0;
}




//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================
// Constructors for dynamic packer types
/** ---------------------------------------------------------------------------
 * creates a bits type packer field.  Always return numeric value.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bits.
 * \lparam   bofs     bit offset from beginning of byte. Default 0.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pck_Bits( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz  = luaL_checkinteger( luaVM, 1 );  ///< how many bits  to read
	int             ofs = luaL_checkinteger( luaVM, 2 );  ///< how many its from starting byte to read

	luaL_argcheck( luaVM,  1<= sz && sz <= 8*8,       1,
	                 "size must be >=1 and <=8" );
	luaL_argcheck( luaVM,  1<= ofs && ofs <= 8,       2,
	                 "offset must be >=1 and <=8" );

	p = xt_pck_create_ud( luaVM, XT_PCK_BITS );

	p->sz  = ((sz+ofs-2)/8)+1;
	p->lB  = sz;
	p->oB  = ofs;
	p->iR  = LUA_NOREF;
	return 1;
}


/** ---------------------------------------------------------------------------
 * creates a string type packer field.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bytes (chars).
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pck_String( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz = luaL_checkinteger( luaVM, 1 );   ///< how many chars in this packer
	luaL_argcheck( luaVM,  1 <= sz , 1,
		                 "size must be >=1" ); 
	p = xt_pck_create_ud( luaVM, XT_PCK_STR );

	p->sz  = sz;
	p->iR  = LUA_NOREF;
	return 1;
}


/**--------------------------------------------------------------------------
 * create a xt_pack and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct xt_pack*  pointer to the  xt_pack struct
 * --------------------------------------------------------------------------*/
struct xt_pck *xt_pck_create_ud( lua_State *luaVM, enum xt_pck_t t)
{
	struct xt_pck  *p;
	p = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ));

	p->t  = t;
	luaL_getmetatable( luaVM, "xt.Pack" );
	lua_setmetatable( luaVM, -2 );
	return p;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an xt_pack struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct xt_pack* pointer to xt_pack struct
 * --------------------------------------------------------------------------*/
struct xt_pck *xt_pck_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Pack" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Pack` expected" );
	return (struct xt_pck *) ud;
}



/**--------------------------------------------------------------------------
 * reads a value, unpacks it and pushes it onto the Lua stack.
 * \param   luaVM lua Virtual Machine.
 * \lparam  struct xt_pack.
 * \lreturn value  unpacked value according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pck_unpack( lua_State *luaVM )
{
	struct xt_pck *p   = xt_pck_check_ud( luaVM, 1 );
	size_t         sL;
	const char    *buf = luaL_checklstring( luaVM, 2, &sL );
	if (sL != p->sz)
		return xt_push_error( luaVM, "Can only unpack data of the size suitable for this packers size" );

	return xt_pck_read( luaVM, p, (const unsigned char *) buf );
}


/**--------------------------------------------------------------------------
 * reads in a Lua value and packs it according to packer format. Return str to Lua Stack
 * \param   luaVM lua Virtual Machine.
 * \lparam  struct xt_pack.
 * \lparam  Lua value.
 * \lreturn string packed value according to packer format.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pck_pack( lua_State *luaVM )
{
	struct xt_pck *p      = xt_pck_check_ud( luaVM, 1 );
	luaL_Buffer    lB;
	char          *buffer;
	int            retVal; ///< return value to evaluate the succes of write operation

	luaL_buffinit( luaVM, &lB );
	buffer = luaL_prepbuffsize( &lB, p->sz );
	memset( buffer, 0, p->sz * sizeof( char ) );

	if ((retVal = xt_pck_write( luaVM, p, (unsigned char *) buffer )) != 0)
	{
		return retVal;
	}
	else
	{
		luaL_pushresultsize( &lB, p->sz );
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Get size in bytes covered by packer/struct/reader.
 * \param   luaVM  The lua state.
 * \lparam  ud     xt.Pack.* instance.
 * \lreturn int    size in bytes.
 * \return  int    The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_size( lua_State *luaVM )
{
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, 1, 0 );
	struct xt_pck  *pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, -1, 1 ) : pr->p;
	lua_pushinteger( luaVM, pc->sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * DEBUG: Get internal reference table from a Struct/Packer.
 * \param   luaVM  The lua state.
 * \lparam  ud     xt.Pack.* instance.
 * \lreturn table  Reference table of all members.
 * \return  int    The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_getir( lua_State *luaVM )
{
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, 1, 0 );
	struct xt_pck  *pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, -1, 1 ) : pr->p;
	if (LUA_NOREF != pc->iR)
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->iR );
	else
		lua_pushnil( luaVM );
	return 1;
}

/**--------------------------------------------------------------------------
 * __tostring helper that prints the packer type.
 * \param   luaVM      The lua state.
 * \param   xt_pack    the packer instance struct.
 * \lreturn  leaves two strings on the Lua Stack.
 * --------------------------------------------------------------------------*/
void xt_pck_format( lua_State *luaVM, struct xt_pck *p )
{
	lua_pushfstring( luaVM, "%s", xt_pck_t_lst[ p->t ] );
	switch( p->t )
	{
		case XT_PCK_INTL:
		case XT_PCK_INTB:
			lua_pushfstring( luaVM, "%d", p->sz );
			break;
		case XT_PCK_BYTE:
			lua_pushfstring( luaVM, "" );          // satisfy lua_concat()
			break;
		case XT_PCK_BIT:
			lua_pushfstring( luaVM, "%d",  p->oB );
			break;
		case XT_PCK_BITS:
			lua_pushfstring( luaVM, "{%d/%d}", p->lB, p->oB );
			break;
		case XT_PCK_NBL:
			lua_pushfstring( luaVM, "%c", (p->oB>4)?'L':'H' );
			break;
		case XT_PCK_STR:
			lua_pushfstring( luaVM, "[%d]", p->sz );
			break;
		case XT_PCK_FLT:
			lua_pushfstring( luaVM, "%d", p->sz );
			break;
		default:
			lua_pushfstring( luaVM, "[%d]", p->n );
	}
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a packer instance.
 * \param   luaVM      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck__tostring( lua_State *luaVM )
{
	struct xt_pck *p = xt_pck_check_ud( luaVM, 1 );
	lua_pushfstring( luaVM, "xt.Pack." );
	xt_pck_format( luaVM, p );
	lua_pushfstring( luaVM, ": %p", p );
	lua_concat( luaVM, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a packer instance.
 * \param   luaVM      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck__len( lua_State *luaVM )
{
	struct xt_pck *p = xt_pck_check_ud( luaVM, 1 );
	lua_pushinteger( luaVM, p->sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Exports all types of xt.Pack as global variables
 * \param   luaVM      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck_export( lua_State *luaVM )
{
	luaL_getsubtable( luaVM, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( luaVM, -1, "xt" );
	lua_getfield( luaVM, -1, "Pack" );
	lua_pushnil( luaVM );
	while (lua_next( luaVM, -2 ))
	{                            // Stack, _LOADED,xt,Pack,name,func
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



/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_pck_cf [] = {
	{"Bits",      lxt_pck_Bits},
	{"String",    lxt_pck_String},
	{"Array",     lxt_pckc_Array},
	{"Sequence",  lxt_pckc_Sequence},
	{"Struct",    lxt_pckc_Struct},
	{"size",      lxt_pck_size},
	{"export",    lxt_pck_export},
	{"get_ref",   lxt_pck_getir},
	{NULL,    NULL}
};


/**
 * \brief   the packer library definition
 *          assigns Lua available names to C-functions
 */
static const luaL_Reg xt_pck_m [] = {
	// new implementation
	{"pack",      lxt_pck_pack},
	{"unpack",    lxt_pck_unpack},
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Pack library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pck( lua_State *luaVM )
{
	
	int            i;                   /// iterator for type creation
	struct xt_pck *t;                   /// type pointer for type creation
	// xt.Pack instance metatable
	luaL_newmetatable( luaVM, "xt.Pack" );   // stack: functions meta
	luaL_newlib( luaVM, xt_pck_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pck__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pck__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as xt.Pack.<member>
	luaL_newlib( luaVM, xt_pck_cf );
	// register static types
	// IntL,IntB
	for (i=1; i<=8; i++)
	{
		lua_pushfstring( luaVM, "IntL%d", i );
		t     = xt_pck_create_ud( luaVM, XT_PCK_INTL );
		t->sz = i;
		lua_rawset( luaVM, -3 );
		lua_pushfstring( luaVM, "IntB%d", i );
		t     = xt_pck_create_ud( luaVM, XT_PCK_INTB );
		t->sz = i;
		t->iR = LUA_NOREF;
		lua_rawset( luaVM, -3 );
		// native endian
		lua_pushfstring( luaVM, "Int%d", i );
		if (IS_LITTLE_ENDIAN)
			lua_pushfstring( luaVM, "IntL%d", i );
		else
			lua_pushfstring( luaVM, "IntB%d", i );
		lua_rawget( luaVM, -3 );
		lua_rawset( luaVM, -3 );

	}
	// Bit1 ..Bit8
	for (i=1; i<=8; i++)
	{
		lua_pushfstring( luaVM, "Bit%d", i );
		t = xt_pck_create_ud( luaVM, XT_PCK_BIT );
		t->sz  = 1;
		t->lB  = 1;
		t->oB  = i;
		t->iR = LUA_NOREF;
		lua_rawset( luaVM, -3 );
	}
	// NibbleL
	lua_pushstring( luaVM, "NibbleL" );
	t = xt_pck_create_ud( luaVM, XT_PCK_NBL );
	t->oB  = 1;
	t->sz  = 1;
	t->iR = LUA_NOREF;
	lua_rawset( luaVM, -3 );
	// NibbleH
	lua_pushstring( luaVM, "NibbleH" );
	t = xt_pck_create_ud( luaVM, XT_PCK_NBL );
	t->oB  = 5;
	t->sz  = 1;
	t->iR = LUA_NOREF;
	lua_rawset( luaVM, -3 );
	// Byte
	lua_pushstring( luaVM, "Byte" );
	t = xt_pck_create_ud( luaVM, XT_PCK_BYTE );
	t->sz  = 1;
	t->iR = LUA_NOREF;
	lua_rawset( luaVM, -3 );
	luaopen_xt_pckc( luaVM );
	luaopen_xt_pckr( luaVM );
	return 1;
}
