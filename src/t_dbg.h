// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_dbg.c
 * \brief     Debug/Development helper functions for lua-t library.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define PRINT_DEBUGS    0
#define UNUSED(...)   (void)(__VA_ARGS__)


void t_fmtStackItem( lua_State *L, int idx, int no_tostring );
void t_stackPrint  ( lua_State *L, int idx, int last, int no_tostring );
void t_stackDump   ( lua_State *L );

