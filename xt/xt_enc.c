/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_enc.c
 * \brief     Umbrella for various En/Decoding routines
 *            This covers encoding/Encryption
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno
#include <unistd.h>      // crypt()
#include <crypt.h>      // crypt()
#include <time.h>       // time()

#include "xt.h"
#include "xt_enc.h"


/** -------------------------------------------------------------------------
 * \brief   Wrapper around crypt() POSIX only
 * \param   luaVM  The lua state.
 * \param   pwd    the password to crypt()
 * \param   string salt (optional)
 * \lreturn encrypted password
 * \lreturn salt
 * \return  void.
 *-------------------------------------------------------------------------*/
static int xt_enc_crypt (lua_State *luaVM)
{
	//char      *salt;
	const char      *pass;

	pass  = luaL_checkstring (luaVM, 1);
	//salt  = luaL_checkstring (luaVM, 2);

	unsigned long seed[2];
	char salt[] = "$1$........";
	const char *const seedchars =
	  "./0123456789ABCDEFGHIJKLMNOPQRST"
	  "UVWXYZabcdefghijklmnopqrstuvwxyz";
	int i;

	/* Generate a (not very) random seed.
		You should do it better than this... */
	seed[0] = time(NULL);
	seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);

	/* Turn it into printable characters from `seedchars'. */
	for (i = 0; i < 8; i++)
	  salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

	/* Read in the user's password and encrypt it. */
	lua_pushstring (luaVM, crypt (pass, salt));
	return 1;
}


/**
 * \brief      the (empty) xt library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_enc_lib [] =
{
	//{"crypt",     xt_enc_crypt},
	{NULL,        NULL}
};


/**
 * \brief     Export the xti enc libray to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_xt_enc (lua_State *luaVM)
{
	luaL_newlib (luaVM, xt_enc_lib);
	luaopen_xt_enc_arc4 (luaVM);
	lua_setfield(luaVM, -2, "Arc4");
	luaopen_xt_enc_crc (luaVM);
	lua_setfield(luaVM, -2, "Crc");
	luaopen_xt_enc_b64 (luaVM);
	lua_setfield(luaVM, -2, "Base64");
	lua_pushcfunction (luaVM, xt_enc_crypt);
	lua_setfield(luaVM, -2, "crypt");
	return 1;
}

