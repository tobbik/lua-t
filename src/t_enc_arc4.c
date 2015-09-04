/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**--------------------------------------------------------------------------
 * \file      t_enc_arc4.c
 * \brief     Arc4 Encryption Decryption algorithm
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 * --------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>      // calloc

#include "t.h"
#include "t_enc.h"


// ----------------------------- Native Arc4 functions
/**--------------------------------------------------------------------------
 * Arc4 Key Scheduler
 * \param struct the state of the Pseudo RNG to be initialized
 * \param key  byte array representing the key
 * \param len  length of the key
 * --------------------------------------------------------------------------*/
static void
t_enc_arc4_init( struct t_enc_arc4 *arc4, const unsigned char *key, size_t kLen )
{
	// int i,j=0;
	// for (i=0; i < 256; ++i)        arc4->prng[i] = i;
	// for (i=0; i < 256; ++i)
	// {
	// 	j = (j + prng[i] + key[i % len]) % 256;
	// 	T_SWAP (unsigned char, prng[i], prng[j]);
	// }

	unsigned char j;
	unsigned int  i;
	// Initialize prng
	for (i = 0; i < 256; i++)      arc4->prng[i] = (unsigned char) i;
	arc4->i1 = 0; arc4->i2 = 0;
	// randomize prng with the key
	for (j = i = 0; i < 256; i++)
	{
		j += arc4->prng[i] + key[i % kLen];
		TSWAP( uint8_t, arc4->prng[i], arc4->prng[j] );
	}
}


/**--------------------------------------------------------------------------
 * Implementation of the ARC4 runner.
 * \param struct The state of the Pseudo RNG to be initialized
 * \param char*  Data input buffer.
 * \param char*  Data output buffer.
 * \param size_t input buffer length.
 * --------------------------------------------------------------------------*/
static void
t_enc_arc4_crypt( struct t_enc_arc4 *arc4, const char *inbuf, char *outbuf, size_t buflen )
{
	uint8_t i,j;

	for (i = 0; i < buflen; i++)
	{
		// Update modification indicies
		arc4->i1 ++;
		arc4->i2 += arc4->prng[arc4->i1];

		// Modify PRNG
		TSWAP (uint8_t,
		        arc4->prng[arc4->i1],
		        arc4->prng[arc4->i2] );

		// crypt next byte
		j = arc4->prng[arc4->i1] + arc4->prng[arc4->i2];
		outbuf[i] = inbuf[i] ^ arc4->prng[j];
	}
}

/*
void av_arc4_crypt(AVARC4 *r, uint8_t *dst, const uint8_t *src, int count, uint8_t *iv, int decrypt) {
	uint8_t x = r->x, y = r->y;
	uint8_t *state = r->state;
	while (count-- > 0) {
		uint8_t sum = state[x] + state[y];
		T_SWAP (uint8_t, state[x], state[y]);
		*dst++ = src ? *src++ ^ state[sum] : state[sum];
		x++;
		y += state[x];
	}
}
*/


// ----------------------------- LUA Arc4 wrapper functions
/**--------------------------------------------------------------------------
 * Construct a Arc4 encoder and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table Arc4
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_arc4__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_enc_arc4_New( L );
}


/**--------------------------------------------------------------------------
 * Construct a Arc4 encoder and return it.
 * \param   L The lua state.
 * \lparam  key   key string (optional)
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional))
 * \lreturn struct arc4 userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_enc_arc4_New( lua_State *L )
{
	struct t_enc_arc4    *arc4;
	const  unsigned char *key;
	size_t                kLen;

	arc4 = t_enc_arc4_create_ud( L );
	if (lua_isstring( L, 1 ))
	{
		key   = (const unsigned char*) luaL_checklstring( L, 1, &kLen );
		t_enc_arc4_init( arc4, key, kLen );
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a arc4 encode userdata and push to LuaStack.
 * \param   L  The lua state.
 * \return  struct t_enc_arc4*  pointer
 * --------------------------------------------------------------------------*/
struct t_enc_arc4
*t_enc_arc4_create_ud( lua_State *L )
{
	struct t_enc_arc4 *arc4;

	arc4 = (struct t_enc_arc4 *) lua_newuserdata( L, sizeof( struct t_enc_arc4 ) );
	luaL_getmetatable( L, "T.Encode.Arc4" );
	lua_setmetatable( L, -2 );
	return arc4;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct Arc4
 * \param   L    The lua state.
 * \param   int      position on the stack
 * \return  struct t_enc_arc4*
 * --------------------------------------------------------------------------*/
struct t_enc_arc4
*t_enc_arc4_check_ud( lua_State *L, int pos )
{
	void *ud = luaL_checkudata( L, pos, "T.Encode.Arc4" );
	luaL_argcheck( L, ud != NULL, pos, "`T.Encode.Arc4` expected" );
	return (struct t_enc_arc4 *) ud;
}


/**--------------------------------------------------------------------------
 * Implements Arc4 cypher.
 * \param  L The Lua state.
 * \lparam body The payload to be encrypted
 * \lparam key  The Arc4 password               OPTIONAL
 *              reinitialize Arc4 state with new key
 * TODO:
 * \lparam iB64  Shall the input  be treaded as Base64
 * \lparam oB64  Shall the output be treaded as Base64
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_arc4_crypt( lua_State *L )
{
	struct t_enc_arc4   *arc4;
	size_t               kLen,bLen;    ///< length of key, length of body
	const unsigned char *key;
	const char          *body;
	char                *res;
	
	arc4 = t_enc_arc4_check_ud( L, 1 );
	if (lua_isstring( L, 2 ))
	{
		body = luaL_checklstring( L, 2, &bLen );
	}
	else
	{
		return t_push_error( L, "T.Arc4.crypt takes at least one string parameter" );
	}
	// if a key is provided for encoding,
	// reset the Arc4 state by initializing it with new key
	if (lua_isstring( L, 3 ))
	{
		key  = (const unsigned char*) luaL_checklstring( L, 3, &kLen );
		t_enc_arc4_init( arc4, key, kLen );
	}

	res = malloc( bLen * sizeof( unsigned char ) );

	t_enc_arc4_crypt( arc4, body, res, bLen );
	lua_pushlstring( L, res, bLen );
	free( res );

	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_arc4_fm [] = {
	{"__call",  lt_enc_arc4__Call},
	{NULL,      NULL}
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_arc4_cf [] = {
	{"new",     lt_enc_arc4_New},
	{NULL,      NULL}
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_enc_arc4_m [] = {
	{"crypt",   lt_enc_arc4_crypt},
	{NULL,      NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the T.Encode.Arc4 library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L       The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_enc_arc4( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, "T.Encode.Arc4" );   // stack: functions meta
	luaL_newlib( L, t_enc_arc4_m );
	lua_setfield( L, -2, "__index" );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Arc4.new
	luaL_newlib( L, t_enc_arc4_cf );
	// set the constructor metatable Arc4()
	luaL_newlib( L, t_enc_arc4_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

