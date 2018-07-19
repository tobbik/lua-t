/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_enc_rc4.c
 * \brief     RC4 Encryption Decryption algorithm.
 *            Yes it's unsafe and considered broken.  Still can be used as a
 *            semi-decent rng.  More of a demonstartion on how to write an
 *            algorthm within Lua-t.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>
#include <stdlib.h>      // calloc

#include "t_enc_l.h"
#include "t.h"           // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif

#define TYPESWAP(type,a,b) do{type SWAP_tmp=b; b=a; a=SWAP_tmp;}while(0)


// ----------------------------- Native RC4 functions
/**--------------------------------------------------------------------------
 * RC4 Key Scheduler
 * \param struct the state of the Pseudo RNG to be initialized
 * \param key    byte array representing the key
 * \param len    length of the key
 * --------------------------------------------------------------------------*/
static void
t_enc_rc4_init( struct t_enc_rc4 *rc4, const unsigned char *key, size_t kLen )
{
	// int i,j=0;
	// for (i=0; i < 256; ++i)        rc4->prng[i] = i;
	// for (i=0; i < 256; ++i)
	// {
	// 	j = (j + prng[i] + key[i % len]) % 256;
	// 	T_SWAP (unsigned char, prng[i], prng[j]);
	// }

	unsigned char j;
	unsigned int  i;
	// Initialize prng
	for (i = 0; i < 256; i++)      rc4->prng[i] = (unsigned char) i;
	rc4->i1 = 0; rc4->i2 = 0;
	// randomize prng with the key
	for (j = i = 0; i < 256; i++)
	{
		j += rc4->prng[i] + key[i % kLen];
		TYPESWAP( uint8_t, rc4->prng[i], rc4->prng[j] );
	}
}


/**--------------------------------------------------------------------------
 * Implementation of the RC4 runner.
 * \param struct The state of the Pseudo RNG to be initialized
 * \param char*  Data input buffer.
 * \param char*  Data output buffer.
 * \param size_t input buffer length.
 * --------------------------------------------------------------------------*/
static void
t_enc_rc4_crypt( struct t_enc_rc4 *rc4, const char *inbuf, char *outbuf, size_t buflen )
{
	uint8_t i,j;

	for (i = 0; i < buflen; i++)
	{
		// Update modification indicies
		rc4->i1 ++;
		rc4->i2 += rc4->prng[rc4->i1];

		// Modify PRNG
		TYPESWAP (uint8_t,
		        rc4->prng[rc4->i1],
		        rc4->prng[rc4->i2] );

		// crypt next byte
		j = rc4->prng[rc4->i1] + rc4->prng[rc4->i2];
		outbuf[i] = inbuf[i] ^ rc4->prng[j];
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


// ----------------------------- LUA RC4 wrapper functions
/**--------------------------------------------------------------------------
 * Construct a RC4 encoder and return it.
 * \param   L     Lua state.
 * \lparam  CLASS table RC4.
 * \lparam  key   key string (optional).
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional)).
 * \lreturn ud    t_enc_rc4 userdata instance.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_rc4__Call( lua_State *L )
{
	struct t_enc_rc4     *rc4;
	const  unsigned char *key;
	size_t                kLen;

	lua_remove( L, 1 );
	rc4 = t_enc_rc4_create_ud( L );
	if (lua_isstring( L, 1 ))
	{
		key   = (const unsigned char*) luaL_checklstring( L, 1, &kLen );
		t_enc_rc4_init( rc4, key, kLen );
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a RC4 encode userdata and push to LuaStack.
 * \param   L      The Lua state.
 * \return  struct t_enc_rc4*  pointer
 * --------------------------------------------------------------------------*/
struct t_enc_rc4
*t_enc_rc4_create_ud( lua_State *L )
{
	struct t_enc_rc4 *rc4;

	rc4 = (struct t_enc_rc4 *) lua_newuserdata( L, sizeof( struct t_enc_rc4 ) );
	luaL_getmetatable( L, T_ENC_RC4_TYPE );
	lua_setmetatable( L, -2 );
	return rc4;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct RC4
 * \param   L        The Lua state.
 * \param   int      position on the stack
 * \return  struct   t_enc_rc4*
 * --------------------------------------------------------------------------*/
struct t_enc_rc4
*t_enc_rc4_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_ENC_RC4_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_ENC_RC4_TYPE );
	return (NULL==ud) ? NULL : (struct t_enc_rc4 *) ud;
}


/**--------------------------------------------------------------------------
 * Implements RC4 cypher.
 * \param         L The Lua state.
 * \lparam string The payload to be encrypted
 * \lparam string The RC4 password               OPTIONAL
 *                reinitialize RC4 state with new key
 * TODO:
 * \lparam iB64   Shall the input  be treaded as Base64
 * \lparam oB64   Shall the output be treaded as Base64
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_rc4_crypt( lua_State *L )
{
	struct t_enc_rc4    *rc4;
	size_t               kLen,bLen;    ///< length of key, length of body
	const unsigned char *key;
	const char          *body;
	char                *res;
	
	rc4 = t_enc_rc4_check_ud( L, 1, 1 );
	if (lua_isstring( L, 2 ))
		body = luaL_checklstring( L, 2, &bLen );
	else
		return luaL_error( L, T_ENC_RC4_TYPE".crypt takes at least one string parameter" );
	// if a key is provided for encoding,
	// reset the RC4 state by initializing it with new key
	if (lua_isstring( L, 3 ))
	{
		key  = (const unsigned char*) luaL_checklstring( L, 3, &kLen );
		t_enc_rc4_init( rc4, key, kLen );
	}

	res = malloc( bLen * sizeof( unsigned char ) );

	t_enc_rc4_crypt( rc4, body, res, bLen );
	lua_pushlstring( L, res, bLen );
	free( res );

	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_rc4_fm [] = {
	  { "__call",  lt_enc_rc4__Call }
	, { NULL,      NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_rc4_cf [] = {
	  { NULL,      NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_enc_rc4_m [] = {
	  { "crypt",   lt_enc_rc4_crypt }
	, { NULL,      NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the T.Encode.RC4 library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The Lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_enc_rc4( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_ENC_RC4_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_enc_rc4_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as RC4.new
	luaL_newlib( L, t_enc_rc4_cf );
	// set the constructor metatable RC4()
	luaL_newlib( L, t_enc_rc4_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

