/**
 * \file   xt_enc_b64.c
 *         A Base64 en/decoder library
 */
#include <stdio.h>
#include <stdlib.h>      // calloc

#include "l_xt.h"
#include "xt_enc.h"

static const char enc_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                 'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                 '4', '5', '6', '7', '8', '9', '+', '/'};

static const char dec_table[] = {-1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, 62, -1, -1, -1, 63,
                                 52, 53, 54, 55, 56, 57, 58, 59,
                                 60, 61, -1, -1, -1, -1, -1, -1,
                                 -1,  0,  1,  2,  3,  4,  5,  6,
                                  7,  8,  9, 10, 11, 12, 13, 14,
                                 15, 16, 17, 18, 19, 20, 21, 22,
                                 23, 24, 25, -1, -1, -1, -1, -1,
                                 -1, 26, 27, 28, 29, 30, 31, 32,
                                 33, 34, 35, 36, 37, 38, 39, 40,
                                 41, 42, 43, 44, 45, 46, 47, 48,
                                 49, 50, 51, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
                                 -1, -1, -1, -1, -1, -1, -1, -1,
};
static const uint32_t  mod_table[] = {0, 2, 1};


// ----------------------------- Native Base64 functions
/**
 * \brief B64 initiator Scheduler
 * \details this is just a dummy to satisfy xt_enc paradigms
 * \param struct the B64 state
 */
static void
b64_init( struct xt_enc_b64 *b64 )
{
	b64->enc_table = enc_table;
	b64->dec_table = dec_table;
}

/**
 * \brief B64 allocator for output
 * \details depending on en or decoding returns empty string of right size
 * \param size of input
 * \param encode, if 1 then return string of size need for encoded result
 */
static size_t
b64_res_size( size_t len, int for_encode )
{
	return (for_encode) ? 4 * ((len + 2) / 3) :  len / 4 * 3;
}


static void
b64_encode( struct xt_enc_b64 *b64, const char *inbuf, char *outbuf, size_t inbuf_len)
{
	uint32_t i,j;
	size_t  outbuf_len = 4 * ((inbuf_len + 2) / 3);

	for (i = 0, j = 0; i < inbuf_len;)
	{
		uint8_t chr1 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;
		uint8_t chr2 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;
		uint8_t chr3 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;

		outbuf[ j++ ] = enc_table[chr1 >> 2];
		outbuf[ j++ ] = enc_table[((chr1 & 3 ) << 4) | (chr2 >> 4)];
		outbuf[ j++ ] = enc_table[((chr2 & 15) << 2) | (chr3 >> 6)];
		outbuf[ j++ ] = enc_table[chr3 & 63];
	}

	for (i = 0; i < mod_table[inbuf_len % 3]; i++)
		outbuf[ outbuf_len - 1 - i ] = '=';
}


// ----------------------------- LUA B64 wrapper functions
/**--------------------------------------------------------------------------
 * construct a B64 encoder and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table Base64
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
xt_enc_b64___call(lua_State *luaVM)
{
	lua_remove( luaVM, 1 );
	return xt_enc_b64_new( luaVM );
}


/**--------------------------------------------------------------------------
 * construct a Base64 encoder and return it.
 * \param   luaVM The lua state.
 * \lparam  key   key string (optional)
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional))
 * \lreturn struct b64 userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
xt_enc_b64_new( lua_State *luaVM )
{
	struct xt_enc_b64    *b64;

	b64 = xt_enc_b64_create_ud( luaVM );
	b64_init( b64 );

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a b64 encode userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_enc_b64*  pointer
 * --------------------------------------------------------------------------*/
struct xt_enc_b64
*xt_enc_b64_create_ud( lua_State *luaVM )
{
	struct xt_enc_b64 *b64;

	b64 = (struct xt_enc_b64 *) lua_newuserdata ( luaVM, sizeof( struct xt_enc_b64 ) );
	luaL_getmetatable( luaVM, "xt.Encode.Base64" );
	lua_setmetatable( luaVM, -2 );
	return b64;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct Base64
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_enc_arc4*
 * --------------------------------------------------------------------------*/
struct xt_enc_b64
*xt_enc_b64_check_ud (lua_State *luaVM, int pos)
{
	void *ud = luaL_checkudata(luaVM, pos, "xt.Encode.Base64");
	luaL_argcheck(luaVM, ud != NULL, pos, "`xt.Encode.Base64` expected");
	return (struct xt_enc_b64 *) ud;
}


/**
 * \brief  implements Base64 cypher.
 * \param  luaVM The Lua state.
 */
static int
xt_enc_b64_encode (lua_State *luaVM)
{
	struct xt_enc_b64  *b64;
	size_t              bLen;    ///< length of body
	size_t              rLen;    ///< length of result
	const char         *body;
	char               *res;
	
	b64 = xt_enc_b64_check_ud (luaVM, 1);
	if (lua_isstring (luaVM, 2)) body = luaL_checklstring(luaVM, 2, &bLen);
	else return xt_push_error(luaVM, "xt.Base64.encode takes at least one string parameter");

	rLen = b64_res_size( bLen, 1 );
	res = malloc( rLen );
	if (res == NULL)
		return xt_push_error( luaVM, "xt.Base64.encode failed due to internal memory allocation problem");

	b64_encode( b64, body, res, bLen);
	lua_pushlstring (luaVM, res, rLen);
	free(res);

	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_enc_b64_fm [] = {
	{"__call",      xt_enc_b64___call},
	{NULL,          NULL}
};


/**
 * \brief      the Base64 static class function library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_enc_b64_cf [] = {
	{"new",     xt_enc_b64_new},
	{NULL,      NULL}
};


/**
 * \brief      the Base64 member functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_enc_b64_m [] =
{
	{"encode",   xt_enc_b64_encode},
	{NULL,      NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the Timer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_xt_enc_b64 (lua_State *luaVM)
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( luaVM, "xt.Encode.Base64" );   // stack: functions meta
	luaL_newlib( luaVM, xt_enc_b64_m );
	lua_setfield( luaVM, -2, "__index");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Base64.new
	luaL_newlib( luaVM, xt_enc_b64_cf );
	// set the constructor metatable Base64()
	luaL_newlib( luaVM, xt_enc_b64_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

