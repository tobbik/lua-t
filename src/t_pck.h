/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck.h
 * \brief     data types for packers
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define T_PCK_IDNT "pck"
#define T_PCK_FLD_IDNT  "fld"

#define T_PCK_NAME "Pack"
#define T_PCK_FLD_NAME  "Field"

#define T_PCK_TYPE "T."T_PCK_NAME
#define T_PCK_FLD_TYPE  T_PCK_TYPE"."T_PCK_FLD_NAME

