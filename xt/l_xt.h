/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_library.h
 * genral helpers for Lua
 *
 * data definitions
 */

#ifdef _WIN32
#include <Windows.h>
#endif

/* includes the Lua headers */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define MAX_PKT_BYTES     1500


void stackDump (lua_State *luaVM);

int pusherror(lua_State *luaVM, const char *info);


LUAMOD_API int luaopen_net    (lua_State *luaVM);
LUAMOD_API int luaopen_buffer (lua_State *luaVM);
LUAMOD_API int luaopen_debug  (lua_State *luaVM);
LUAMOD_API int luaopen_xt     (lua_State *luaVM);
