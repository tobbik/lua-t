/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/** -------------------------------------------------------------------------
 * \file      t_tst.h
 * \brief     Data Types and global functions for T.Test
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *-------------------------------------------------------------------------*/

#define T_TST_STE_NAME  "Suite"
#define T_TST_CSE_NAME  "Case"

#define T_TST_STE_TYPE  T_TST_TYPE"."T_TST_STE_NAME
#define T_TST_CSE_TYPE  T_TST_TYPE"."T_TST_CSE_NAME

void t_tst_check_ud( lua_State *L, int pos );
