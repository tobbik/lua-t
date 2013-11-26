/**
 * \file   library.c
 *         a couple of helper files used throughout all library files
 */
#include <stdio.h>
#include <stdlib.h>      // calloc

#include "l_xt.h"
#include "xt_enc.h"


// ----------------------------- Native Arc4 functions
/**
 * \brief Arc4 Key Scheduler
 * \param struct the state of the Pseudo RNG to be initialized
 * \param key  byte array representing the key
 * \param len  length of the key
 */
static void
arc4_init (struct xt_enc_arc4 *arc4, const unsigned char *key, size_t kLen)
{
	// int i,j=0;
	// for (i=0; i < 256; ++i)        arc4->prng[i] = i;
	// for (i=0; i < 256; ++i)
	// {
	// 	j = (j + prng[i] + key[i % len]) % 256;
	// 	XT_SWAP (unsigned char, prng[i], prng[j]);
	// }

	unsigned char j;
	unsigned int  i;
	// Initialize prng
	for (i = 0; i < 256; i++)      arc4->prng[i] = (unsigned char) i;
	arc4->i1 = 0; arc4->i2 = 0;
	// randomize prng with the key
	for (j = i = 0; i < 256; i++) {
		j += arc4->prng[i] + key[i % kLen];
		XTSWAP (uint8_t, arc4->prng[i], arc4->prng[j]);
	}
}


static void
arc4_crypt(struct xt_enc_arc4 *arc4, const char *inbuf, char *outbuf, size_t buflen)
{
	uint8_t i,j;

	for (i = 0; i < buflen; i++)
	{
		// Update modification indicies
		arc4->i1 ++;
		arc4->i2 += arc4->prng[arc4->i1];

		// Modify PRNG
		XTSWAP (uint8_t,
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
		XT_SWAP (uint8_t, state[x], state[y]);
		*dst++ = src ? *src++ ^ state[sum] : state[sum];
		x++;
		y += state[x];
	}
}
*/


// ----------------------------- LUA Arc4 wrapper functions
/**--------------------------------------------------------------------------
 * construct a Arc4 encoder and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table Arc4
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int 
xt_enc_arc4___call(lua_State *luaVM)
{
	lua_remove(luaVM, 1);
	return xt_enc_arc4_new(luaVM);
}


/**--------------------------------------------------------------------------
 * construct a Arc4 encoder and return it.
 * \param   luaVM The lua state.
 * \lparam  key   key string (optional)
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional))
 * \lreturn struct arc4 userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
xt_enc_arc4_new(lua_State *luaVM)
{
	struct xt_enc_arc4    *arc4;
	const  unsigned char *key;
	size_t                kLen;

	arc4 = xt_enc_arc4_create_ud (luaVM);
	if (lua_isstring (luaVM, 1)) {
		key   = (const unsigned char*) luaL_checklstring(luaVM, 1, &kLen);
		arc4_init (arc4, key, kLen);
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a arc4 encode userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_enc_arc4*  pointer
 * --------------------------------------------------------------------------*/
struct xt_enc_arc4
*xt_enc_arc4_create_ud(lua_State *luaVM)
{
	struct xt_enc_arc4 *arc4;

	arc4 = (struct xt_enc_arc4 *) lua_newuserdata (luaVM, sizeof(struct xt_enc_arc4) );
	luaL_getmetatable(luaVM, "xt.Encode.Arc4");
	lua_setmetatable(luaVM, -2);
	return arc4;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct Arc4
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_enc_arc4*
 * --------------------------------------------------------------------------*/
struct xt_enc_arc4
*xt_enc_arc4_check_ud (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "xt.Encode.Arc4");
	luaL_argcheck(luaVM, ud != NULL, pos, "`xt.Encode.Arc4` expected");
	return (struct xt_enc_arc4 *) ud;
}


/**
 * \brief  implements Arc4 cypher.
 * \param  luaVM The Lua state.
 * \lparam key  The Arc4 password
 * \lparam body The payload to be encrypted
 * \lparam iB64  Shall the input be treaded as Base64
 * \lparam oB64  Shall the output be treaded as Base64
 */
static int
xt_enc_arc4_crypt (lua_State *luaVM) {
	struct xt_enc_arc4 *arc4;
	size_t              kLen,bLen;    ///< length of key, length of body
	const char         *key;
	const char         *body;
	char               *res;
	
	arc4 = xt_enc_arc4_check_ud (luaVM, 1);
	if (lua_isstring (luaVM, 3)) key  = luaL_checklstring(luaVM, 3, &kLen);
	if (lua_isstring (luaVM, 2)) body = luaL_checklstring(luaVM, 2, &bLen);
	else return xt_push_error(luaVM, "xt.Arc4.crypt takes at least one string parameter");

	res = malloc (bLen * sizeof (unsigned char));

	arc4_crypt(arc4, body, res, bLen);
	lua_pushlstring (luaVM, res, bLen);
	free(res);

	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_enc_arc4_fm [] = {
	{"__call",      xt_enc_arc4___call},
	{NULL,          NULL}
};


/**
 * \brief      the Arc4 static class function library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_enc_arc4_cf [] = {
	{"new",     xt_enc_arc4_new},
	{NULL,      NULL}
};


/**
 * \brief      the Arc4 member functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_enc_arc4_m [] =
{
	{"crypt",   xt_enc_arc4_crypt},
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
int luaopen_enc_arc4 (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable(luaVM, "xt.Encode.Arc4");   // stack: functions meta
	luaL_newlib(luaVM, xt_enc_arc4_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Arc4.new
	luaL_newlib(luaVM, xt_enc_arc4_cf);
	// set the constructor metatable Arc4()
	luaL_newlib(luaVM, xt_enc_arc4_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

