//
//
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "library.h"
#include "l_buffer.h"


// inline helper functions
/**
 * \brief     convert 8bit integer to BCD
 * \param     val  8bit integer
 * \return    8bit integer encoding of a 2 digit BCD number
 */
int l_byteToBcd(lua_State *luaVM)
{
	uint8_t val = luaL_checkint(luaVM, 1);
	lua_pushinteger(luaVM, (val/10*16) + (val%10) );
	return 1;
}

/**
 * \brief     convert 16bit integer to BCD
 * \param     val  16bit integer
 * \return    16bit integer encoding of a BCD coded number
 */
int l_shortToBcd(lua_State *luaVM)
{
	uint16_t val = luaL_checkint(luaVM, 1);
	lua_pushinteger(luaVM,
			(val/1000   *4096 ) +
		 ( (val/100%10)* 256 ) +
		 ( (val/10%10) * 16 ) +
			(val%10)
	);
	return 1;
}


/**
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_buffer_fm [] =
{
	{NULL,        NULL}
};


/**
 * \brief     Export the net library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_buffer (lua_State *luaVM)
{
	luaL_newlib (luaVM, l_buffer_fm);
	luaopen_buffer_stream(luaVM);
	lua_setfield(luaVM, -2, "Stream");
	return 1;
}

