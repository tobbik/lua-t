/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/** -------------------------------------------------------------------------
 * \file      t_enc.c
 * \brief     Umbrella for various En/Decoding routines
 *            This covers encoding/Encryption
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *-------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno
#include <unistd.h>     // crypt()
#include <crypt.h>      // crypt()
#include <time.h>       // time()

#include "t.h"
#include "t_enc.h"


/** -------------------------------------------------------------------------
 * Wrapper around crypt() POSIX only
 * \param   L      Lua state.
 * \lparam  string password to crypt()
 * \lparam  string salt (optional)
 * \lreturn string encrypted password
 * \lreturn string salt
 * \return  void.
 *-------------------------------------------------------------------------*/
static int
lt_enc_crypt( lua_State *L )
{
	//char      *salt;
	const char    *pass;

	pass  = luaL_checkstring( L, 1 );
	//salt  = luaL_checkstring( L, 2 );

	unsigned long seed[2];
	char salt[] = "$1$........";
	const char *const seedchars =
	  "./0123456789ABCDEFGHIJKLMNOPQRST"
	  "UVWXYZabcdefghijklmnopqrstuvwxyz";
	int i;

	// Generate a (not very) random seed. You should do it better than this...
	seed[ 0 ] = time( NULL );
	seed[ 1 ] = getpid( ) ^ (seed[0] >> 14 & 0x30000);

	// Turn it into printable characters from `seedchars'.
	for (i = 0; i < 8; i++)
	{
		salt[3+i] = seedchars[ (seed[i/5] >> (i%5)*6) & 0x3f ];
	}

	// Read in the user's password and encrypt it.
	lua_pushstring( L, crypt( pass, salt ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_enc_cf [] =
{
	//  { "crypt",     lt_enc_crypt }
	, { NULL,        NULL }
};


/** -------------------------------------------------------------------------
 * Export the t_enc libray to Lua
 * \param   L   The Lua state.
 * \return  int # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_enc( lua_State *L )
{
	luaL_newlib( L, t_enc_cf );
	luaopen_t_enc_rc4( L );
	lua_setfield( L, -2, T_ENC_RC4_NAME );
	luaopen_t_enc_crc( L );
	lua_setfield( L, -2, T_ENC_CRC_NAME );
	luaopen_t_enc_b64( L );
	lua_setfield( L, -2, T_ENC_B64_NAME );
	lua_pushcfunction( L, lt_enc_crypt );
	lua_setfield( L, -2, "crypt" );
	return 1;
}

