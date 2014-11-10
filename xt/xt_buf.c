/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_buf.c
 * \brief     OOP wrapper for a universal buffer implementation
 *            Allows for reading writing as mutable string
 *            can be used for socket communication
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset

#include "xt.h"
#include "xt_buf.h"


// --------------------------------- HELPERS Functions

/** -------------------------------------------------------------------------
 * Helper to check arguments for being a xt_buf and a valid position
 * \param     luaVM  lua state.
 * \param     pB     position on stack which is buffer.
 * \param     pB     position on stack which is position.
 *                   handled as pointer and decremented by one to deal with the
 *                   fact that C char buffers indexes are zero based.
 * \return   *xt_buf pointer to validated buffer.
 *  -------------------------------------------------------------------------*/

struct xt_buf * xt_buf_getbuffer( lua_State *luaVM, int pB, int pP, int *pos )
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, pB, 1 );

	*pos = luaL_checkint( luaVM, pP );   ///< starting byte  b->b[pos]

	luaL_argcheck( luaVM,  1 <= *pos && *pos <= (int) buf->len, 2,
		                 "xt.Buffer position must be >= 1 or <= #buffer" );
	*pos = *pos-1;     /// account for array access being 0 based
	return buf;
}



/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////
/** -------------------------------------------------------------------------
 * \brief     creates the buffer from the Constructor
 * \param     luaVM  lua state
 * \lparam    CLASS table Time
 * \lparam    length of buffer
 * \lparam    string buffer content initialized            OPTIONAL
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int lxt_buf__Call (lua_State *luaVM)
{
	lua_remove( luaVM, 1 );    // remove the xt.Buffer Class table
	return lxt_buf_New( luaVM );
}


/** -------------------------------------------------------------------------
 * \brief     creates the buffer from the function call
 * \param     luaVM  lua state
 * \lparam    length of buffer
 *        ALTERNATIVE
 * \lparam    string buffer content initialized
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
int lxt_buf_New( lua_State *luaVM )
{
	size_t                                   sz;
	struct xt_buf  __attribute__ ((unused)) *buf;

	if (lua_isnumber( luaVM, 1))
	{
		sz  = luaL_checkint( luaVM, 1 );
		buf = xt_buf_create_ud( luaVM, sz );
	}
	else if (lua_isstring( luaVM, 1 ))
	{
		luaL_checklstring( luaVM, 1, &sz);
		buf = xt_buf_create_ud( luaVM, sz );
		memcpy( (char*) &(buf->b[0]), luaL_checklstring( luaVM, 1, NULL ), sz );
	}
	else
	{
		xt_push_error( luaVM, "can't create xt.Buffer because of wrong argument type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * create a xt_buf and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct xt_buf*  pointer to the  xt_buf struct
 * --------------------------------------------------------------------------*/
struct xt_buf *xt_buf_create_ud( lua_State *luaVM, int size )
{
	struct xt_buf  *b;
	size_t          sz;

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct xt_buf ) + (size - 1) * sizeof( unsigned char );
	b  = (struct xt_buf *) lua_newuserdata( luaVM, sz );
	memset( b->b, 0, size * sizeof( unsigned char ) );

	b->len = size;
	luaL_getmetatable( luaVM, "xt.Buffer" );
	lua_setmetatable( luaVM, -2 );
	return b;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an xt_buf struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct xt_buf* pointer to xt_buf struct
 * --------------------------------------------------------------------------*/
struct xt_buf *xt_buf_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "xt.Buffer" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`xt.Buffer` expected" );
	return (NULL==ud) ? NULL : (struct xt_buf *) ud;
}


//////////////////////////////////////////////////////////////////////////////////////
/////////////// NEW IMPLEMENTATION

// =========+Buffer accessor Helpers
//
/**--------------------------------------------------------------------------
 * Read an integer of y bytes from a char buffer pointer
 * General helper function to read the value of an 64 bit integer from a char array
 * \param   sz         how many bytes to read.
 * \param   islittle   treat input as little endian?
 * \param   buf        pointer to char array to read from.
 * \return  val        integer value.
 * --------------------------------------------------------------------------*/
uint64_t xt_buf_readbytes( size_t sz, int islittle, const unsigned char * buf )
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
void xt_buf_writebytes( uint64_t val, size_t sz, int islittle, unsigned char * buf )
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
uint64_t xt_buf_readbits( size_t len, size_t ofs, const unsigned char * buf )
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
void xt_buf_writebits( uint64_t val, size_t len, size_t ofs, unsigned char * buf )
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

//
// ================================= GENERIC LUA API========================

/**--------------------------------------------------------------------------
 * Read a value of type x from the buffer at pos y.
 * \lparam  ud   xt.Buffer (struct xt_buf).
 * \lparam  ud   xt.Pack (struct xt_pck).
 * \lparam  pos  position in bytes.
 * \lreturn val  Lua value.
 *
 * \return integer 1 left on the stack.
 * --------------------------------------------------------------------------*/
static int lxt_buf_read( lua_State *luaVM )
{
	int            pos;                               ///< starting byte  b->b[pos]
	struct xt_buf *buf = xt_buf_getbuffer( luaVM, 1 , 2, &pos);
	struct xt_pck *pck = xt_pck_check_ud( luaVM, 3 );

	return xt_pck_read( luaVM, pck, &(buf->b[ pos ]));
}


/**--------------------------------------------------------------------------
 * Write a value to the buffer as defined in the xt.Pack argument.
 * \lparam  ud   xt.Buffer (struct xt_buf).
 * \lparam  ud   xt.Pack (struct xt_pck).
 * \lparam  pos  position in bytes.
 * \lparam  val  Lua value to write to buffer.
 *
 * \return integer 0 left on the stack.
 * --------------------------------------------------------------------------*/
static int lxt_buf_write( lua_State *luaVM )
{
	int            pos;                               ///< starting byte  b->b[pos]
	struct xt_buf *buf = xt_buf_getbuffer( luaVM, 1 , 2, &pos);
	struct xt_pck *pck = xt_pck_check_ud( luaVM, 3 );
	int            ret;

	if ((ret = xt_pck_write( luaVM, pck, &(buf->b[ pos ]) )) != 0)
		return ret; // xt_push_error result
	return 0;
}


/**--------------------------------------------------------------------------
 * Read a set of chars to the buffer at position x.
 * \lparam  buf  userdata of type xt.Buffer (struct xt_buf).
 * \lparam  pos  position in bytes.
 * \lparam  sz   size in bytes(1-#buf).
 * \lreturn val  lua_String.
 * TODO: check buffer length vs requested size and offset
 *
 * \return integer 1 left on the stack.
 * --------------------------------------------------------------------------*/
static int lxt_buf_readraw (lua_State *luaVM)
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1, 1 );
	int            pos;
	int            sz;

	pos = (lua_isnumber(luaVM, 2)) ? (size_t) luaL_checkint( luaVM, 2 ) : 0;
	sz  = (lua_isnumber(luaVM, 3)) ? (size_t) luaL_checkint( luaVM, 3 ) : buf->len - pos;

	lua_pushlstring(luaVM, (const char*) &(buf->b[ pos ]), sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Write an set of chars to the buffer at position x.
 * \lparam  buf  userdata of type xt.Buffer (struct xt_buf).
 * \lparam  val  lua_String.
 * \lparam  pos  position in bytes.
 * \lparam  sz   size in bits (1-64).
 * TODO: check string vs buffer length
 *
 * \return integer 1 left on the stack.
 * --------------------------------------------------------------------------*/
static int lxt_buf_writeraw (lua_State *luaVM)
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1, 1 );
	int            pos = (lua_isnumber(luaVM, 3)) ? luaL_checkint(luaVM, 3) : 0;
	size_t         sz;

	// if a third parameter is given write only x bytes of the input string to the buffer
	if (lua_isnumber(luaVM, 4))
	{
		sz  = luaL_checkint(luaVM, 4);
		memcpy  ( (char*) &(buf->b[ pos ]), luaL_checklstring( luaVM, 2, NULL ), sz );
	}
	// otherwise write the whole thing
	else
	{
		memcpy  ( (char*) &(buf->b[ pos ]), luaL_checklstring( luaVM, 2, &sz ), sz );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief    gets the content of the Stream in Hex
 * \lreturn  string buffer representation in Hexadecimal
 * \TODO     use luaL_Buffer?
 *
 * \return integer 0 left on the stack
 * --------------------------------------------------------------------------*/
static int lxt_buf_tohexstring (lua_State *luaVM)
{
	int            l,c;
	char          *sbuf;
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1, 1 );

	sbuf = malloc( 3 * buf->len * sizeof( char ) );
	memset( sbuf, 0, 3 * buf->len * sizeof( char ) );

	c = 0;
	for (l=0; l < (int) buf->len; l++) {
		c += snprintf( sbuf+c, 4, "%02X ", buf->b[l] );
	}
	lua_pushstring( luaVM, sbuf );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief     returns len of the buffer
 * \param     lua state
 * \return    integer   how many elements are placed on the Lua stack
 * --------------------------------------------------------------------------*/
static int lxt_buf__len (lua_State *luaVM)
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1, 1 );

	lua_pushinteger( luaVM, (int) buf->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer stream.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the buffer-Stream in user_data.
 * \lreturn string    formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_buf__tostring (lua_State *luaVM)
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM, "xt.Buffer{%d}: %p", buf->len, buf );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg xt_buf_fm [] = {
	{"__call",        lxt_buf__Call},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg xt_buf_cf [] = {
	{"new",           lxt_buf_New},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg xt_buf_m [] = {
	{"read",       lxt_buf_read},
	{"write",      lxt_buf_write},
	{"readRaw",    lxt_buf_readraw},
	{"writeRaw",   lxt_buf_writeraw},
	// univeral stuff
	{"toHex",      lxt_buf_tohexstring},
	{"length",     lxt_buf__len},
	{"toString",   lxt_buf__tostring},
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the BufferBuffer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_buf (lua_State *luaVM)
{
	// xt.Buffer instance metatable
	luaL_newmetatable( luaVM, "xt.Buffer" );
	luaL_newlib( luaVM, xt_buf_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_buf__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lxt_buf__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	// xt.Buffer class
	luaL_newlib( luaVM, xt_buf_cf );
	luaL_newlib( luaVM, xt_buf_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}


