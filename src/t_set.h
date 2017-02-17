/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_oht.h
 * \brief     data types for Sets.
 *            Elements of a Set are unordered (like a bag).  Each element can
 *            exist only once.  Implemented as a mapper around a Lua table.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// t_set.c
// Constructors
int  luaopen_t_set ( lua_State *L );
int  t_set_check   ( lua_State *L, int pos, int check );
int  t_set_create  ( lua_State *L );

