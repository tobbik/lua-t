/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/** -------------------------------------------------------------------------
 * \file      t_tst.h
 * \brief     Data Types and global functions for T.Test
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *-------------------------------------------------------------------------*/

#define T_TST_CSE_NAME  "Case"

#define T_TST_CSE_TYPE  T_TST_TYPE"."T_TST_CSE_NAME
#define T_TST_CSE_SKIPINDICATOR  "<test_case_skip_indicator>:"

int    t_tst_create( lua_State *L );
int    t_tst_check( lua_State *L, int pos, int check );
LUA_API int luaopen_t_tst( lua_State *L );

int    t_tst_cse_create( lua_State *L );
int    t_tst_cse_check( lua_State *L, int pos, int check );
void   t_tst_cse_addTapDiagnostic( lua_State *L, luaL_Buffer *lB, int pos );
void   t_tst_cse_getDescription( lua_State *L, int pos );
int    t_tst_cse_hasField( lua_State *L, const char *fld, int leave );
int    t_tst_isReallyEqual( lua_State *L );
LUA_API int luaopen_t_tst_cse( lua_State *L );
