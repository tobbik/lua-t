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

#include <sys/select.h>
#include <stdint.h>

/* includes the Lua headers */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define MAX_PKT_BYTES     1500

#define PRINT_DEBUGS      0


// http://stackoverflow.com/questions/2100331/c-macro-definition-to-determine-big-endian-or-little-endian-machine
// http://esr.ibiblio.org/?p=5095
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)
#define IS_LITTLE_ENDIAN (1 == *(unsigned char *)&(const int){1})
//#define IS_LITTLE_ENDIAN (*(uint16_t*)"\0\1">>8)
//#define IS_BIG_ENDIAN (*(uint16_t*)"\1\0">>8)



#define XTSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)


void stackDump       (lua_State *luaVM);
int  xt_push_error   (lua_State *luaVM, const char *fmt, ...);


// global helper functions
void     make_fdset(lua_State *luaVM, int stack_pos, fd_set *collection, int *max_hndl);
uint16_t get_crc16(const unsigned char *data, size_t size);


// global sub classes registration
LUAMOD_API int luaopen_xt_time     (lua_State *luaVM);
LUAMOD_API int luaopen_ipendpoint  (lua_State *luaVM);
LUAMOD_API int luaopen_socket      (lua_State *luaVM);
LUAMOD_API int luaopen_xt_enc      (lua_State *luaVM);
LUAMOD_API int luaopen_xt_buf      (lua_State *luaVM);
LUAMOD_API int luaopen_xt_pack     (lua_State *luaVM);
LUAMOD_API int luaopen_xt_test     (lua_State *luaVM);
LUAMOD_API int luaopen_debug       (lua_State *luaVM);
LUAMOD_API int luaopen_xt          (lua_State *luaVM);
