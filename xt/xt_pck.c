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

// Constructors for packer types
/** ---------------------------------------------------------------------------
 * creates a byte type packer field with little Endian.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bytes.
 * \lparam   islittle treat as litlle endian?
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_IntL( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz = luaL_checkint( luaVM, 1 );   ///< how many bytes to read
	luaL_argcheck( luaVM,  1<= sz && sz <= 8,       1,
		                 "size must be >=1 and <=8" );
	p = xt_pck_create_ud( luaVM, XT_PCK_INTL );

	p->sz    = sz;
	return 1;
}


/** ---------------------------------------------------------------------------
 * creates a byte type packer field with Big Endian.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bytes.
 * \lparam   islittle treat as litlle endian?
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_IntB( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz = luaL_checkint( luaVM, 1 );   ///< how many bytes to read
	luaL_argcheck( luaVM,  1<= sz && sz <= 8,       1,
		                 "size must be >=1 and <=8" );
	p = xt_pck_create_ud( luaVM, XT_PCK_INTB );

	p->sz    = sz;
	return 1;
}


/** ---------------------------------------------------------------------------
 * creates a bit type packer field.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bits.
 *         OPTIONAL
 * \lparam   bofs     bit offset from beginning of byte. Default 0.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_Bit( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz = luaL_checkint( luaVM, 1 );  ///< how many bits  to read
	int             ofs = 0;                         ///< how many its from starting byte to read

	luaL_argcheck( luaVM,  1<= sz && sz <= 8*8,       1,
		                 "size must be >=1 and <=8" );
	if (lua_isnumber( luaVM, 2 ))
	{
		ofs = luaL_checkint( luaVM, 2 );
		luaL_argcheck( luaVM,  0<= ofs && ofs <= 8,       2,
		                 "offset must be >=0 and <=8" );
	}

	p = xt_pck_create_ud( luaVM, XT_PCK_BIT );

	p->sz  = ((sz+ofs-1)/8)+1;
	p->lB  = sz;
	p->oB  = ofs;
	return 1;
}


/** ---------------------------------------------------------------------------
 * creates a string type packer field.
 * \param    luaVM    lua state.
 * \lparam   sz       size of packer in bytes (chars).
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_String( lua_State *luaVM )
{
	struct xt_pck  *p;
	int             sz = luaL_checkint( luaVM, 1 );   ///< how many chars in this packer
	luaL_argcheck( luaVM,  1<= sz , 1,
		                 "size must be >=1" ); 
	p = xt_pck_create_ud( luaVM, XT_PCK_STR );

	p->sz    = sz;
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


////////////////////////////////////////////////////---------------ACCESSORS

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
		case XT_PCK_BIT:
			lua_pushinteger( luaVM, (lua_Integer) xt_buf_readbits( p->lB, p->oB, b ) );
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
			intVal = luaL_checkint( luaVM, -1 );
			luaL_argcheck( luaVM,  intVal  <  0x01 << (p->sz*8), -1,
			              "value to pack must be smaller than the maximum value for the packer size");
			xt_buf_writebytes( (uint64_t) intVal, p->sz, p->t, b );
			break;
		case XT_PCK_BIT:
			intVal = luaL_checkint( luaVM, -1 );
			luaL_argcheck( luaVM,  intVal  <  0x01 << p->lB, -1,
			              "value to pack must be smaller than the maximum value for the packer size");
			xt_buf_writebits( (uint64_t) intVal, p->lB, p->oB, b );
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
	p->oR = 0;
	return 0;
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
 * reads a Lua Value from an xt.Buffer/String according to the Packer.
 * \param   luaVM lua Virtual Machine.
 * \lparam  userdata xt.Pack.
 * \lparam  userdata xt.Buffer.
 * \lreturn value    from xt.Buffer/String according to packer format.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pck_read( lua_State *luaVM )
{
	struct xt_pck *p      = xt_pckc_check_ud( luaVM, 1, 1 );  // allow Pack or Struct
	struct xt_buf *b      = xt_buf_check_ud( luaVM, 2 );

	if (p->t < XT_PCK_STRUCT)       // xt.Pack value type -> return result
	{
		return xt_pck_read( luaVM, p, (const unsigned char *) b->b );
	}
	else                            // xt.Pack.Struct comb type -> return table
	{
		return 0;;
	}
}



/**--------------------------------------------------------------------------
 * Get the number of elements in any packer/struct value.
 * \param   luaVM   The lua state.
 * \lparam  xt.Pack.* instance.
 * \lreturn int   size in bytes.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_len( lua_State *luaVM )
{
	struct xt_pck   *p = xt_pckc_check_ud( luaVM, -1, 1 );   // Allow for both packer or struct
	lua_pushinteger( luaVM, p->n );
	return 1;
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

	switch( p->t )
	{
		case XT_PCK_INTL:
			lua_pushfstring( luaVM, "xt.Pack{INTL:%d}: %p", p->sz, p );
			break;
		case XT_PCK_INTB:
			lua_pushfstring( luaVM, "xt.Pack{INTB}: %p", p );
			break;
		case XT_PCK_BIT:
			lua_pushfstring( luaVM, "xt.Pack{BIT[%d/%d]:%d}: %p", p->lB, p->oB, p->sz, p );
			break;
		case XT_PCK_STR:
			lua_pushfstring( luaVM, "xt.Pack{STRING:%d}: %p", p->sz, p );
			break;
		case XT_PCK_FLT:
			lua_pushfstring( luaVM, "xt.Pack{FLOAT:%d}: %p", p->sz, p );
			break;
		default:
			xt_push_error( luaVM, "Can't read value from unknown packer type" );
	}
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


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_pck_cf [] = {
	{"Bit",       lxt_pck_Bit},
	{"Int",       lxt_pck_IntL},
	{"IntL",      lxt_pck_IntL},
	{"IntB",      lxt_pck_IntB},
	{"String",    lxt_pck_String},
	{"Struct",    lxt_pckc_Struct},
	{"Array",     lxt_pckc_Array},
	{"len",       lxt_pck_len},
	{"read",      lxt_pck_read},
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
	luaopen_xt_pckc( luaVM );
	luaopen_xt_pckr( luaVM );
	return 1;
}
