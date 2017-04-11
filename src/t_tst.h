/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/** -------------------------------------------------------------------------
 * \file      t_tst.h
 * \brief     Data Types and global functions for T.Test
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *-------------------------------------------------------------------------*/
// Asynchronous test case execution step
enum t_tst_cse_state {
	T_TST_CSE_BFE,        ///< beforeEach()
	T_TST_CSE_EXC,        ///< test case function()
	T_TST_CSE_AFE,        ///> afterEach()
	T_TST_CSE_DNE,        ///> done()
};


#define T_TST_CSE_IDNT  "cse"

#define T_TST_CSE_NAME  "Case"

#define T_TST_CSE_TYPE  T_TST_TYPE"."T_TST_CSE_NAME
#define T_TST_CSE_SKIPINDICATOR  "<test_case_skip_indicator>:" // must have trailing ":"

int    t_tst_create( lua_State *L );
int    t_tst_check( lua_State *L, int pos, int check );
int    t_tst_done( lua_State *L );
LUA_API int luaopen_t_tst( lua_State *L );

int    t_tst_cse_create( lua_State *L );
int    t_tst_cse_check( lua_State *L, int pos, int check );
void   t_tst_cse_addTapDiagnostic( lua_State *L, int pos );
void   t_tst_cse_getDescription( lua_State *L, int pos );
int    t_tst_cse_hasField( lua_State *L, const int pos, const char *fld, int leave );
int    t_tst_cse_isType( lua_State *L, int pos, const char *typeName );
int    lt_tst_cse__call( lua_State *L );
int    t_tst_cse_prune( lua_State *L );
LUA_API int luaopen_t_tst_cse( lua_State *L );
