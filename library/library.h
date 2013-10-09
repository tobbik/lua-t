/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_library.h
 * genral helpers for Lua
 *
 * data definitions
 */

#ifdef _WIN32
#define QTC_TARGET_WIN32  1
#define QTC_TARGET_FAMILY_WINDOWS  1
#else
#define QTC_TARGET_LINUX  1
#endif
#include "qtc_types.h"

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
