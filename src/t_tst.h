/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tst.h
 * \brief     Data Types for T.Test
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define T_TST_IDNT      "tst"
#define T_TST_CSE_IDNT  "cse"

#define T_TST_NAME      "Test"
#define T_TST_CSE_NAME  "Case"

#define T_TST_TYPE      "T."T_TST_NAME
#define T_TST_CSE_TYPE  T_TST_TYPE"."T_TST_CSE_NAME

#define T_TST_CSE_NAME  "Case"

#define T_TST_CSE_TYPE  T_TST_TYPE"."T_TST_CSE_NAME
#define T_TST_CSE_SKIPINDICATOR  "<test_case_skip_indicator>:" // must have trailing ":"


